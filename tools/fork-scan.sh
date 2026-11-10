#!/usr/bin/env bash

# SPDX-FileCopyrightText: Copyright 2026 Eglantine contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# Lists commits other Eglantine forks made that Orchid does not have yet, as Markdown.
# Run after every release; the list is the start of the next release's fork ports.
#
#   tools/fork-scan.sh [--since <date>] [--base <ref>]
#
# --since  only commits committed after this date (default: the release before the newest
#          v* tag on the base, so each release's scan overlaps the previous one)
# --base   the branch to compare against (default: HEAD)
#
# Needs git and an authenticated gh. A commit counts as known when its short hash appears in a
# commit message on the base (ports cite their source as "<fork> <hash>"), when the base has a
# commit with the same subject or patch, or when .github/fork-scan-skip lists it (one hash and
# a reason per line, for commits looked at and turned down).

set -euo pipefail

# Root repositories: their own branch is fetched and compared in full, and their GitHub forks are
# compared against them. Mirrors and non-GitHub forks go here too.
ROOTS=(
    "zortos293/Eglantine nx2"
    "Djihads80/Eglantine nx2"
    "sylvieistaken/selevet-eglantine"
    "Apollo422/PokeDinner"
)
SELF="MythosOfs/Orchid"

since=""
base="HEAD"
while [ $# -gt 0 ]; do
    case "$1" in
    --since) since="$2"; shift 2 ;;
    --base) base="$2"; shift 2 ;;
    *) echo "usage: $0 [--since <date>] [--base <ref>]" >&2; exit 2 ;;
    esac
done

command -v gh > /dev/null || { echo "gh is missing" >&2; exit 1; }
cd "$(git rev-parse --show-toplevel)"

if [ -z "$since" ]; then
    previous=$(git tag --merged "$base" --sort=-creatordate --list 'v*' | sed -n 2p)
    [ -n "$previous" ] || { echo "no previous v* tag on $base, pass --since" >&2; exit 1; }
    since=$(git log -1 --format=%cI "$previous")
fi
since_epoch=$(date -d "$since" +%s)
since_utc=$(date -u -d "$since" +%Y-%m-%dT%H:%M:%SZ)

# Everything the base already references: cited hashes (7+ hex chars), subjects, skip list.
known=$(mktemp)
seen=$(mktemp)
trap 'rm -f "$known" "$seen"' EXIT
{
    git log "$base" --format=%B | grep -oE '\b[0-9a-f]{7,40}\b' | cut -c1-7 || true
    git log "$base" --format='S %s'
    if [ -f .github/fork-scan-skip ]; then
        grep -oE '^[0-9a-f]{7,40}' .github/fork-scan-skip | cut -c1-7 || true
    fi
} | sort -u > "$known"

is_known() { # <sha> <subject>
    grep -qxF "${1:0:7}" "$known" || grep -qxF "S $2" "$known"
}

print_commit() { # <repo> <sha> <epoch> <author> <subject> [<branch>]
    [ "$3" -ge "$since_epoch" ] || return 0
    is_known "$2" "$5" && return 0
    # A commit shows up once, under the first repository that has it.
    grep -qxF "$2" "$seen" && return 0
    echo "$2" >> "$seen"
    # shellcheck disable=SC2016 # the backticks are Markdown
    printf -- '- [`%s`](https://github.com/%s/commit/%s) %s, %s: %s%s\n' \
        "${2:0:9}" "$1" "$2" "$(date -d "@$3" +%F)" "$4" "$5" "${6:+ ($6)}"
}

echo "# Fork scan against \`$(git rev-parse --short "$base")\`, commits since $(date -d "$since" +%F)"
echo

# 1. The roots' own branches, by git. --cherry-pick drops commits whose patch the base has.
for entry in "${ROOTS[@]}"; do
    read -r repo branch <<< "$entry"
    ref="refs/fork-scan/${repo//\//-}"
    git fetch -q "https://github.com/$repo.git" "+${branch:-HEAD}:$ref"
    out=$(git log --no-merges --cherry-pick --right-only --since="$since" \
        --format='%H%x09%ct%x09%an%x09%s' "$base...$ref" |
        while IFS=$'\t' read -r sha epoch author subject; do
            print_commit "$repo" "$sha" "$epoch" "$author" "$subject"
        done)
    if [ -n "$out" ]; then
        echo "## $repo${branch:+ ($branch)}"
        echo
        echo "$out"
        echo
    fi
done

# 2. GitHub forks of the first two roots, every branch pushed after the fork was made, compared
# against the root's default branch. Forks not pushed to since the fork was made or since the
# window starts, and branches that still point where one of the root's branches does (forks copy
# all of them), are skipped unasked. Forks run in parallel (one API call per branch), each into
# its own file; the files are printed in fork order and a commit two forks share is kept once.
scan_fork() { # <root> <root branch> <root heads> <fork>
    local out
    out=$(gh api --paginate "repos/$4/branches?per_page=100" \
        -q '.[] | [.name, .commit.sha] | @tsv' |
        while IFS=$'\t' read -r branch head; do
            grep -qxF "$head" <<< "$3" && continue
            # shortcut: compare lists at most 250 commits; a fork that far ahead of its
            # root needs a look by hand anyway.
            gh api "repos/$4/compare/${1%%/*}:$2...$branch" \
                -q '.commits[] | select(.parents | length == 1) |
                    [.sha, .commit.committer.date, .commit.author.name,
                     (.commit.message | split("\n")[0])] | @tsv' 2> /dev/null |
                while IFS=$'\t' read -r sha date author subject; do
                    # An API error (rate limit) comes back as JSON, not as commits.
                    if [[ ! $sha =~ ^[0-9a-f]{40}$ ]]; then
                        echo "fork-scan: $4 $branch: ${sha:0:100}" >&2
                        continue
                    fi
                    print_commit "$4" "$sha" "$(date -d "$date" +%s)" \
                        "$author" "$subject" "$branch"
                done
        done | sort -u)
    if [ -n "$out" ]; then
        echo "## $4"
        echo
        echo "$out"
        echo
    fi
}

parts=$(mktemp -d)
trap 'rm -rf "$known" "$seen" "$parts"' EXIT
jobs_max=8
n=0
for entry in "${ROOTS[@]:0:2}"; do
    read -r root root_branch <<< "$entry"
    root_heads=$(gh api --paginate "repos/$root/branches?per_page=100" -q '.[].commit.sha')
    while read -r fork; do
        [ "$fork" = "$SELF" ] && continue
        while [ "$(jobs -rp | wc -l)" -ge "$jobs_max" ]; do wait -n || true; done
        scan_fork "$root" "$root_branch" "$root_heads" "$fork" > "$parts/$(printf %05d "$n")" &
        n=$((n + 1))
    done < <(gh api --paginate "repos/$root/forks?per_page=100" \
        -q ".[] | select(.pushed_at > .created_at and .pushed_at >= \"$since_utc\") | .full_name")
done
wait
# Parallel forks race on $seen: drop repeats here, and sections that end up empty.
cat "$parts"/* 2> /dev/null | awk '
    /^## / { flush(); head = $0; body = ""; next }
    /^- \[`/ { sha = $0; sub(/^[^(]*\(https:\/\/github.com\/[^\/]*\/[^\/]*\/commit\//, "", sha)
               sub(/\).*/, "", sha); if (sha in done) next; done[sha] = 1; body = body $0 "\n"; next }
    function flush() { if (body != "") printf "%s\n\n%s\n", head, body; body = "" }
    END { flush() }'

echo "Mark turned-down commits in \`.github/fork-scan-skip\` so the next scan leaves them out."

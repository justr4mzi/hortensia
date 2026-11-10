# Fork scan

Other Eglantine forks fix things Orchid lacks. After every release, the next one starts with
their fixes.

1. The release workflow's last job runs `tools/fork-scan.sh` and opens a `fork-scan` issue,
   "Fork scan after v0.x.y". It lists, per fork, the commits made since the previous release
   that `main` does not have and does not cite yet. Run it by hand the same way:
   `tools/fork-scan.sh --since 2026-11-01 > scan.md`.
2. Look at each commit. Port a fix that holds up (correct, general, not already done another way
   in `main`) as its own commit: `<subsystem>: <summary>`, then what was wrong and what changes,
   then the credit line, `Ported from <fork> <hash> (<author>).`, or `Adapted from ...` with what
   was left out. The short hash in that line is how later scans know the commit is taken.
3. A commit looked at and turned down goes into `.github/fork-scan-skip` as `<hash> <reason>`, so
   later scans leave it out.
4. Name the fork and what came from it under Credits in `README.md`.
5. Build, run the tests and the Mario Tennis Fever menu gate, push to `main`, close the issue
   with a list of what was ported. The next release ships it.

Roots are listed at the top of the script: zortos293/Eglantine and Djihads80/Eglantine (each with
their GitHub forks), Selevet Eglantine and the PokeDinner mirror. Add a fork there when it lives
somewhere the GitHub fork lists don't reach.

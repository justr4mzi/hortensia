#!/bin/sh -e
# SPDX-FileCopyrightText: Copyright 2026 Orchid Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Linux development build. Extra arguments are passed to CMake's configure step.
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
build=${ORCHID_BUILD_DIR:-"$root/build-local"}
configure_only=false
if [ "${1:-}" = --configure-only ]; then
    configure_only=true
    shift
fi

for tool in cmake ninja ccache; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "Missing $tool. Install cmake, ninja and ccache first." >&2
        exit 1
    }
done

# Keep the cache across branches/worktrees; normalize source paths in debug info.
export CCACHE_BASEDIR="$root"
export CCACHE_NOHASHDIR=true
export CCACHE_DIR="${CCACHE_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/orchid/ccache}"

# Large renderer translation units need substantial RAM. Leave room for the desktop.
jobs=${CMAKE_BUILD_PARALLEL_LEVEL:-}
if [ -z "$jobs" ]; then
    jobs=$(awk '/MemAvailable:/ { n = int(($2 - 2097152) / 2097152); print (n > 0 ? n : 1) }' /proc/meminfo)
    cpus=$(getconf _NPROCESSORS_ONLN)
    [ "$jobs" -le "$cpus" ] || jobs=$cpus
fi

# Reconfiguring unconditionally updates build timestamps and forces needless links.
# Ninja handles changes to CMake inputs; explicit options request a reconfigure.
if [ ! -f "$build/CMakeCache.txt" ] || [ "$configure_only" = true ] || [ "$#" -gt 0 ]; then
    cmake -S "$root" -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    "-DCMAKE_C_FLAGS_RELWITHDEBINFO=-O2 -g1 -DNDEBUG -fdebug-prefix-map=$root=." \
    "-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-O2 -g1 -DNDEBUG -fdebug-prefix-map=$root=." \
    -DUSE_CCACHE=ON -DUSE_FASTER_LINKER=ON -DENABLE_LTO=OFF \
    -DBUILD_TESTING=OFF -DYUZU_TESTS=OFF -DYUZU_CMD=OFF \
    -DYUZU_ROOM_STANDALONE=OFF \
    -DVulkanHeaders_FORCE_BUNDLED=ON -DVulkanUtilityLibraries_FORCE_BUNDLED=ON \
    "$@"
fi

if [ "$configure_only" = false ]; then
    echo "Building emulator with $jobs parallel jobs"
    cmake --build "$build" --target yuzu --parallel "$jobs"
    echo "Run: $build/bin/orchid"
fi

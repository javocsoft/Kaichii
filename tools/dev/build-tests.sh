#!/bin/sh -e

# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Configures, builds and runs the C++ tests with the flags .github/workflows/linux-extended.yml uses.
# Meant for the container, where a first build takes about 40 minutes on 12 jobs:
#
#   tools/dev/in-docker.sh tools/dev/build-tests.sh [gcc|clang] [ctest arguments]
#
# LICHII_JOBS sets the parallel jobs (default 12); a job can need 2 GB of memory.

cd "$(dirname "$0")/../.."

compiler=clang
case "${1:-}" in
    gcc | clang)
        compiler=$1
        shift
        ;;
esac

if [ "$compiler" = gcc ]; then
    cc=gcc-13
    cxx=g++-13
else
    cc=clang-20
    cxx=clang++-20
fi

build=build/$compiler

cmake -S . -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER="$cc" \
    -DCMAKE_CXX_COMPILER="$cxx" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DENABLE_WERROR=ON \
    -DBUILD_TESTING=ON \
    -DYUZU_TESTS=ON \
    -DDYNARMIC_TESTS=ON \
    -DENABLE_DEBUG_TOOLS=ON \
    -DENABLE_QT=ON \
    -DYUZU_CMD=ON \
    -DYUZU_ROOM=OFF \
    -DYUZU_ROOM_STANDALONE=OFF \
    -DYUZU_USE_BUNDLED_QT=OFF \
    -DCMAKE_PREFIX_PATH="$QT_ROOT_DIR" \
    -DYUZU_USE_BUNDLED_SDL3=ON \
    -DVulkanHeaders_FORCE_BUNDLED=ON \
    -DVulkanUtilityLibraries_FORCE_BUNDLED=ON \
    -DENABLE_UPDATE_CHECKER=ON \
    -DUSE_DISCORD_PRESENCE=OFF \
    -DUSE_CCACHE=ON

cmake --build "$build" --parallel "${LICHII_JOBS:-12}"

ctest --test-dir "$build" --output-on-failure --no-tests=error "$@"

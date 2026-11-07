#!/bin/sh -e

# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Configures, builds and runs the C++ tests with the flags .github/workflows/linux-extended.yml uses.
# Meant for the container, where a first build takes about 15 minutes on 12 jobs:
#
#   tools/dev/in-docker.sh tools/dev/build-tests.sh [gcc|clang] [ctest arguments]
#
# LICHII_JOBS sets the parallel jobs (default 12); a job can need 2 GB of memory.
#
# LICHII_SANITIZE=asan (AddressSanitizer and UBSan) or tsan builds only the test programs, with Clang,
# in build/clang-<name>, and does not run them. See docs/ai/testing.md for how to.

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
werror=ON
flags=
targets=

case "${LICHII_SANITIZE:-}" in
    "") ;;
    # The tree has no RTTI for vptr, and the function check reads before each call target, which
    # faults when generated code starts a mapping. Recovering reports every error in one run.
    asan) flags="-fsanitize=address,undefined -fno-sanitize=vptr,function -fsanitize-recover=address" ;;
    tsan) flags="-fsanitize=thread" ;;
    *)
        echo "LICHII_SANITIZE must be asan or tsan" >&2
        exit 1
        ;;
esac

if [ -n "$flags" ]; then
    cc=clang-20
    cxx=clang++-20
    build=build/clang-$LICHII_SANITIZE
    flags="$flags -g1 -fno-omit-frame-pointer"
    # Sanitized code warns where the normal build does not, and that is not what this build checks.
    werror=OFF
    targets="--target tests --target dynarmic_tests"
fi

cmake -S . -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="$flags" \
    -DCMAKE_CXX_FLAGS="$flags" \
    -DCMAKE_EXE_LINKER_FLAGS="$flags" \
    -DCMAKE_C_COMPILER="$cc" \
    -DCMAKE_CXX_COMPILER="$cxx" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DENABLE_WERROR="$werror" \
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

# shellcheck disable=SC2086 # Each --target is its own word.
cmake --build "$build" --parallel "${LICHII_JOBS:-12}" $targets

# Sanitized programs need the options in docs/ai/testing.md, which ctest would not give them.
[ -z "$flags" ] || exit 0

ctest --test-dir "$build" --output-on-failure --no-tests=error "$@"

#!/bin/sh -e

# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Measures line coverage of the C++ tests and ranks the riskiest gaps (docs/ai/testing.md).
# Meant for the container; the first build takes as long as tools/dev/build-tests.sh:
#
#   tools/dev/in-docker.sh tools/dev/coverage.sh [coverage_report.py arguments]

cd "$(dirname "$0")/../.."

build=build/coverage
flags="-fprofile-instr-generate -fcoverage-mapping"

cmake -S . -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang-20 \
    -DCMAKE_CXX_COMPILER=clang++-20 \
    -DCMAKE_C_FLAGS="$flags" \
    -DCMAKE_CXX_FLAGS="$flags" \
    -DCMAKE_C_FLAGS_RELEASE="-O1 -DNDEBUG" \
    -DCMAKE_CXX_FLAGS_RELEASE="-O1 -DNDEBUG" \
    -DCMAKE_EXE_LINKER_FLAGS=-fprofile-instr-generate \
    -DENABLE_WERROR=OFF \
    -DBUILD_TESTING=ON \
    -DYUZU_TESTS=ON \
    -DDYNARMIC_TESTS=ON \
    -DENABLE_DEBUG_TOOLS=ON \
    -DENABLE_QT=ON \
    -DYUZU_CMD=OFF \
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

rm -rf "$build/profiles"
LLVM_PROFILE_FILE="$PWD/$build/profiles/%p-%m.profraw" \
    ctest --test-dir "$build" --no-tests=error --output-on-failure

llvm-profdata merge -sparse "$build"/profiles/*.profraw -o "$build/all.profdata"

bin=$build/bin
llvm-cov export -summary-only -instr-profile="$build/all.profdata" "$bin/tests" \
    -object "$bin/dynarmic_tests" -object "$bin/config_tests" \
    -object "$bin/update_checker_tests" -object "$bin/compatibility_report_tests" \
    -ignore-filename-regex='(\.cache/|externals/|/build/|src/tests/|src/dynarmic/tests/|tools/)' \
    > "$build/coverage.json" 2>/dev/null

python3 tools/dev/coverage_report.py "$build/coverage.json" "$@"

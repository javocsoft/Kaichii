#!/bin/sh -e

# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Runs the checks in .clang-tidy over the emulator. Meant for the container:
#
#   tools/dev/in-docker.sh tools/dev/tidy.sh [regex of files]
#
# Needs a configured build/clang with its generated headers, which tools/dev/build-tests.sh leaves.
# The whole tree takes about an hour on 12 jobs. Findings go to build/clang/tidy.log.

cd "$(dirname "$0")/../.."

files=${1:-src/(common|core|video_core|shader_recompiler|network|audio_core|hid_core|input_common|frontend_common)/.*\.cpp$}

run-clang-tidy-20 -quiet -p build/clang -j "${LICHII_JOBS:-12}" "$files" > build/clang/tidy.log 2>&1 || true

grep -E '(warning|error): ' build/clang/tidy.log | sort -u | wc -l

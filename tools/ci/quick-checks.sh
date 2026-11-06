#!/bin/sh -e

# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# The fast CI gate: nothing here compiles the emulator. Same commands locally and in
# .github/workflows/quick-checks.yml. Run tools/dev/in-docker.sh tools/ci/quick-checks.sh for every
# linter without installing them. A linter that is not installed is skipped with a notice.
#
#   tools/ci/quick-checks.sh [base commit, default the merge base with origin/nx2]

cd "$(dirname "$0")/../.."

base=${1:-$(git merge-base origin/nx2 HEAD)}
failed=

# Runs every check instead of stopping at the first, so one push reports everything.
fail() {
    echo "-- FAILED: $1"
    failed="$failed $1"
}

python3 tools/ci/check_repo.py --base "$base" || fail check_repo

# Suites that need Vulkan run in .github/workflows/linux-extended.yml instead. Listing what to skip,
# not what to run, means a new suite joins the gate on its own.
for suite in tools/ci/test_*.py tools/nx2/test_*.py; do
    case "$suite" in
        */test_fsr3_shaders.py | */test_buffer_host_visibility.py) continue ;;
    esac
    if output=$(PYTHONDONTWRITEBYTECODE=1 python3 "$suite" 2>&1); then
        echo "-- $suite: $(echo "$output" | tail -n 3 | tr '\n' ' ')"
    else
        echo "$output"
        fail "$suite"
    fi
done

# The inherited tree fails these (791 codespell hits, 11 shell scripts), so they cover changed files.
changed=$(git diff --name-only --diff-filter=ACMR "$base...HEAD" -- \
    ':!externals' ':!.patch' ':!dist' ':!src/android/app/src/main/res')

scripts=$(for file in $changed; do
    case "$file" in
        *.sh | hooks/*) echo "$file" ;;
    esac
done)

if command -v shellcheck >/dev/null; then
    # shellcheck disable=SC2086
    [ -z "$scripts" ] || shellcheck -S warning $scripts || fail shellcheck
else
    echo "-- shellcheck not installed, skipped"
fi

if command -v codespell >/dev/null; then
    # shellcheck disable=SC2086
    [ -z "$changed" ] || codespell --quiet-level=3 $changed || fail codespell
else
    echo "-- codespell not installed, skipped"
fi

if command -v actionlint >/dev/null; then
    actionlint -shellcheck= -pyflakes= || fail actionlint
else
    echo "-- actionlint not installed, skipped"
fi

if [ -n "$failed" ]; then
    echo "quick-checks: failed:$failed"
    exit 1
fi
echo "quick-checks: all good"

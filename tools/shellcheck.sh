#!/bin/sh -e

# SPDX-FileCopyrightText: Copyright 2025 crueter
# SPDX-License-Identifier: GPL-3.0-or-later

# Skips vcvarsall.sh, which is vendored, and mkw_race_bench.sh, which is zsh.
# do NOT use fd in scripts, PLEASE
find tools .ci -name "*.sh" ! -name vcvarsall.sh ! -name mkw_race_bench.sh \
    -exec shellcheck -S warning hooks/pre-commit {} +

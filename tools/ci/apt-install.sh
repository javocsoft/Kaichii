#!/bin/bash
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Installs packages on a GitHub runner: apt-get update, then install without recommends.
#
#   tools/ci/apt-install.sh pkg1 pkg2 ...
#
# A runner whose DNS stopped resolving the Ubuntu mirrors kept apt retrying every package with long
# timeouts until the job's own limit cancelled it (90 minutes, on 2026-11-11). Short timeouts make
# a broken network fail in minutes instead, and the whole install is tried three times, since such
# failures are often brief. Workflows also give the step a timeout of its own.
set -u

options=(-o Acquire::Retries=2 -o Acquire::http::Timeout=15 -o Acquire::https::Timeout=15
         -o DPkg::Lock::Timeout=120)
for attempt in 1 2 3; do
    if sudo apt-get "${options[@]}" update &&
        sudo apt-get "${options[@]}" install -y --no-install-recommends "$@"; then
        exit 0
    fi
    if [ "$attempt" -lt 3 ]; then
        echo "apt-install: attempt $attempt failed, trying again in 30 s" >&2
        sleep 30
    fi
done
echo "apt-install: could not install the packages after 3 attempts" >&2
exit 1

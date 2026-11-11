#!/bin/bash
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Installs packages on a GitHub runner: apt-get update, then install without recommends.
#
#   APT_CACHE_DIR=<dir> tools/ci/apt-install.sh pkg1 pkg2 ...
#
# With APT_CACHE_DIR (restored and saved by actions/cache), the downloaded packages and apt's
# package lists are kept there. A warm cache installs without touching the network; only a cold
# one downloads, and the workflows save it right after. On 2026-11-11 the Ubuntu mirrors served a
# job's ~234 MB at 350 kB/s, and runs failed or timed out while downloading.
#
# A failed install is tried three times, 30 s apart, with short apt timeouts, since mirror and DNS
# failures are often brief. Workflows also give the step a timeout of its own.
set -u

options=(-o Acquire::Retries=3 -o Acquire::http::Timeout=30 -o Acquire::https::Timeout=30
         -o DPkg::Lock::Timeout=120)
cache=${APT_CACHE_DIR:-}
if [ -n "$cache" ]; then
    sudo mkdir -p "$cache/archives/partial" "$cache/lists/partial"
    options+=(-o "Dir::Cache::archives=$cache/archives" -o "Dir::State::lists=$cache/lists")
fi

# Lets actions/cache read what apt (as root) wrote.
own_cache() {
    if [ -n "$cache" ]; then
        sudo chown -R "$(id -u):$(id -g)" "$cache"
    fi
}

install() {
    sudo apt-get "${options[@]}" install -y --no-install-recommends "$@"
}

if [ -n "$cache" ] && compgen -G "$cache/lists/*_Packages*" > /dev/null; then
    if install "$@"; then
        echo "apt-install: installed from the package cache" >&2
        own_cache
        exit 0
    fi
    echo "apt-install: the package cache was not enough, updating the lists" >&2
fi

for attempt in 1 2 3; do
    if sudo apt-get "${options[@]}" update && install "$@"; then
        own_cache
        exit 0
    fi
    if [ "$attempt" -lt 3 ]; then
        echo "apt-install: attempt $attempt failed, trying again in 30 s" >&2
        sleep 30
    fi
done
echo "apt-install: could not install the packages after 3 attempts" >&2
exit 1

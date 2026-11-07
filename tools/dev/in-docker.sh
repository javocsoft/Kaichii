#!/bin/sh -e

# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Runs a command inside the Linux build image with the repository at /src, as the calling user.
# Build output belongs under build/ and caches under .cache/, both ignored by git.
#
#   tools/dev/in-docker.sh ctest --test-dir build/gcc
#
# LICHII_DOCKER_MEMORY caps the container (default 24g). A full build needs about 2 GB per job.

root=$(cd "$(dirname "$0")/../.." && pwd)
image=lichii-dev:latest

if ! docker image inspect "$image" >/dev/null 2>&1; then
    docker build -t "$image" "$root/tools/dev"
fi

mkdir -p "$root/.cache/ccache" "$root/.cache/home"

[ -t 0 ] && [ -t 1 ] && tty=-it || tty=
# shellcheck disable=SC2086
exec docker run --rm $tty \
    --user "$(id -u):$(id -g)" \
    --memory "${LICHII_DOCKER_MEMORY:-24g}" \
    -v "$root:/src" -w /src \
    -e HOME=/src/.cache/home \
    -e CCACHE_DIR=/src/.cache/ccache \
    -e CCACHE_MAXSIZE=30G \
    "$image" "$@"

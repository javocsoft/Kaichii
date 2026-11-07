# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Linux build environment matching .github/workflows/build-check.yml, for hosts without the
# toolchain. See docs/ai/building.md; tools/dev/in-docker.sh runs commands inside it.
FROM ubuntu:24.04

ARG CMAKE_VERSION=4.4.4
ARG QT_VERSION=6.8.3
ARG LLVM_VERSION=20
ARG ACTIONLINT_VERSION=1.7.12
ARG ACTIONLINT_SHA256=8aca8db96f1b94770f1b0d72b6dddcb1ebb8123cb3712530b08cc387b349a3d8

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf ca-certificates ccache curl file g++-13 gcc-13 git glslang-tools \
    libasound2-dev libavcodec-dev libavfilter-dev libboost-context-dev libboost-dev \
    libboost-fiber-dev libegl1 libgl-dev libglib2.0-0 libglu1-mesa-dev libopus-dev \
    libpulse-dev libssl-dev libswscale-dev libtool libudev-dev libusb-1.0-0-dev libva-dev \
    libvdpau-dev libvulkan1 libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 \
    libxcb-randr0 libxcb-render-util0 libxcb-shape0 libxcb-xinerama0 libxcb-xkb1 \
    libxkbcommon-x11-0 make mesa-common-dev mesa-vulkan-drivers nasm ninja-build patch perl \
    pkg-config python3 python3-numpy python3-pil python3-venv shellcheck spirv-headers \
    spirv-tools unzip vulkan-validationlayers xz-utils zip zlib1g-dev zstd \
    clang-${LLVM_VERSION} lld-${LLVM_VERSION} llvm-${LLVM_VERSION} \
    libclang-rt-${LLVM_VERSION}-dev \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv --system-site-packages /opt/venv \
    && /opt/venv/bin/pip install --no-cache-dir cmake==${CMAKE_VERSION} aqtinstall codespell vulkan==1.3.275.1 \
    && /opt/venv/bin/aqt install-qt linux desktop ${QT_VERSION} linux_gcc_64 -m qtcharts -O /opt/qt

RUN curl -fsSLo /tmp/actionlint.tar.gz \
        https://github.com/rhysd/actionlint/releases/download/v${ACTIONLINT_VERSION}/actionlint_${ACTIONLINT_VERSION}_linux_amd64.tar.gz \
    && echo "${ACTIONLINT_SHA256}  /tmp/actionlint.tar.gz" | sha256sum -c - \
    && tar -xzf /tmp/actionlint.tar.gz -C /usr/local/bin actionlint \
    && rm /tmp/actionlint.tar.gz

ENV PATH=/opt/venv/bin:/usr/lib/llvm-${LLVM_VERSION}/bin:$PATH
ENV QT_ROOT_DIR=/opt/qt/${QT_VERSION}/gcc_64
ENV CCACHE_COMPILERCHECK=content
ENV CXX=g++-13

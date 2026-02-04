#!/bin/bash
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

apt-get update && \
  apt-get install -y --no-install-recommends \
  build-essential \
  binutils-dev \
  python3 \
  cmake \
  meson \
  ruby \
  autotools-dev \
  autoconf \
  libtool \
  git \
  curl \
  llvm-dev \
  libclang-dev \
  clang \
  patchelf \
  zlib1g-dev

rm -rf /var/lib/apt/lists/*

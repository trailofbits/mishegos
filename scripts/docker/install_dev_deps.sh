#!/bin/bash
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

apt-get update && \
  apt-get install -y --no-install-recommends \
  gpg wget ca-certificates curl && \
  wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
  gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
  echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ noble main' | \
  tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
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
  llvm-dev \
  libclang-dev \
  clang \
  patchelf

rm -rf /var/lib/apt/lists/*

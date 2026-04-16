#!/bin/bash
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

apt-get -o Acquire::Retries=3 update
apt-get install -y --no-install-recommends \
  binutils \
  llvm \
  ruby \
  zlib1g
rm -rf /var/lib/apt/lists/*

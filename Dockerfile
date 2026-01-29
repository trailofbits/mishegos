# ==============================================================================
# Stage 1: deps - Build dependencies base image
# ==============================================================================
FROM ubuntu:20.04 AS deps

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        gpg wget ca-certificates curl && \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
        gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main' | \
        tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        binutils-dev \
        python \
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
        patchelf && \
    rm -rf /var/lib/apt/lists/*

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal
ENV PATH="/root/.cargo/bin:${PATH}"

WORKDIR /app/mishegos

# ==============================================================================
# Stage 2: build - Compile all components
# ==============================================================================
FROM deps AS build

ARG TARGET=all

COPY ./ .

RUN make "${TARGET}" -j $(nproc)

# Prepare artifacts directory mirroring final deploy structure
RUN mkdir -p /artifacts/src/mishegos /artifacts/src/mish2jsonl /artifacts/workers /artifacts/lib \
             /artifacts/src/analysis /artifacts/src/mishmat \
             /artifacts/src/worker/ghidra/build/sleigh-cmake/specfiles/Ghidra/Processors/x86/data/languages && \
    cp src/mishegos/mishegos /artifacts/src/mishegos/ && \
    cp src/mish2jsonl/mish2jsonl /artifacts/src/mish2jsonl/ && \
    cp workers.spec /artifacts/ && \
    find src/worker -name "*.so" -exec cp {} /artifacts/workers/ \; && \
    cp src/worker/xed/xed/kits/xed-mishegos/lib/libxed.so /artifacts/lib/ 2>/dev/null || true && \
    cp src/worker/capstone/capstone/libcapstone.so.5 /artifacts/lib/ 2>/dev/null || true && \
    cp -r src/analysis/* /artifacts/src/analysis/ && \
    cp -r src/mishmat/* /artifacts/src/mishmat/ && \
    cp src/worker/ghidra/build/sleigh-cmake/specfiles/Ghidra/Processors/x86/data/languages/x86-64.sla \
       src/worker/ghidra/build/sleigh-cmake/specfiles/Ghidra/Processors/x86/data/languages/x86-64.pspec \
       /artifacts/src/worker/ghidra/build/sleigh-cmake/specfiles/Ghidra/Processors/x86/data/languages/ 2>/dev/null || true && \
    patchelf --set-rpath '$ORIGIN/../lib' /artifacts/workers/xed.so 2>/dev/null || true && \
    patchelf --set-rpath '$ORIGIN/../lib' /artifacts/workers/capstone.so 2>/dev/null || true && \
    sed -i 's|^\./src/worker/[^/]*/|./workers/|g' /artifacts/workers.spec

# ==============================================================================
# Stage 3: deploy - Minimal runtime image
# ==============================================================================
FROM ubuntu:20.04 AS deploy

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        binutils \
        llvm \
        ruby && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app/mishegos

# Copy all artifacts in one command
COPY --from=build /artifacts/ ./

ENV LD_LIBRARY_PATH="/app/mishegos/lib"

CMD ["/bin/bash"]

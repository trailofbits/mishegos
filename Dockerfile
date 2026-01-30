# ==============================================================================
# Stage 1: deps - Build dependencies base image
# ==============================================================================
FROM ubuntu:24.04 AS dev

COPY scripts/docker/install_dev_deps.sh /tmp/install_dev_deps.sh
RUN bash /tmp/install_dev_deps.sh

USER ubuntu
WORKDIR /home/ubuntu

COPY scripts/docker/install_rust.sh /tmp/install_rust.sh
RUN bash /tmp/install_rust.sh

ENV PATH="/home/ubuntu/.cargo/bin:${PATH}"

WORKDIR /workspace

# ==============================================================================
# Stage 2: build - Compile all components
# ==============================================================================
FROM dev AS build

COPY --chown=ubuntu:ubuntu . .

RUN make all -j $(nproc)

# Prepare artifacts directory mirroring final deploy structure
# TODO: Integrate this into a 'make install' command or something
RUN mkdir -p ~/artifacts/src/mishegos ~/artifacts/src/mish2jsonl ~/artifacts/workers ~/artifacts/lib \
             ~/artifacts/src/analysis ~/artifacts/src/mishmat \
             ~/artifacts/src/worker/ghidra/build/sleigh-cmake/specfiles/Ghidra/Processors/x86/data/languages && \
    cp src/mishegos/mishegos ~/artifacts/src/mishegos/ && \
    cp src/mish2jsonl/mish2jsonl ~/artifacts/src/mish2jsonl/ && \
    cp workers.spec ~/artifacts/ && \
    find src/worker -name "*.so" -exec cp {} ~/artifacts/workers/ \; && \
    cp src/worker/xed/xed/kits/xed-mishegos/lib/libxed.so ~/artifacts/lib/ 2>/dev/null || true && \
    cp src/worker/capstone/capstone/libcapstone.so.5 ~/artifacts/lib/ 2>/dev/null || true && \
    cp -r src/analysis/* ~/artifacts/src/analysis/ && \
    cp -r src/mishmat/* ~/artifacts/src/mishmat/ && \
    cp src/worker/ghidra/build/sleigh-cmake/specfiles/Ghidra/Processors/x86/data/languages/x86-64.sla \
       src/worker/ghidra/build/sleigh-cmake/specfiles/Ghidra/Processors/x86/data/languages/x86-64.pspec \
       ~/artifacts/src/worker/ghidra/build/sleigh-cmake/specfiles/Ghidra/Processors/x86/data/languages/ 2>/dev/null || true && \
    patchelf --set-rpath '$ORIGIN/../lib' ~/artifacts/workers/xed.so 2>/dev/null || true && \
    patchelf --set-rpath '$ORIGIN/../lib' ~/artifacts/workers/capstone.so 2>/dev/null || true && \
    sed -i 's|^\./src/worker/[^/]*/|./workers/|g' ~/artifacts/workers.spec

# ==============================================================================
# Stage 3: devcontainer - Development environment for VS Code
# ==============================================================================
FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04 AS devcontainer

ENV DEVCONTAINER=true

COPY scripts/docker/install_dev_deps.sh /tmp/install_dev_deps.sh
RUN bash /tmp/install_dev_deps.sh

# Mounts and config for persisting sessions across container rebuilds
RUN SNIPPET="export PROMPT_COMMAND='history -a' && export HISTFILE=/commandhistory/.bash_history" \
    && mkdir /commandhistory \
    && touch /commandhistory/.bash_history \
    && chown -R vscode /commandhistory \
    && echo "$SNIPPET" >> "/home/vscode/.bashrc"

USER vscode
WORKDIR /home/vscode

# Install user tools
COPY scripts/docker/install_rust.sh /tmp/install_rust.sh
RUN bash /tmp/install_rust.sh

WORKDIR /workspace

# ==============================================================================
# Stage 4: deploy - Minimal runtime image
# ==============================================================================
FROM ubuntu:24.04 AS deploy

COPY scripts/docker/install_runtime_deps.sh /tmp/install_runtime_deps.sh
RUN bash /tmp/install_runtime_deps.sh

USER ubuntu

WORKDIR /app/mishegos

# Copy all artifacts in one command
COPY --from=build /home/ubuntu/artifacts/ .

ENV LD_LIBRARY_PATH="/app/mishegos/lib"

CMD ["/bin/bash"]

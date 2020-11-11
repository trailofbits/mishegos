FROM ubuntu:latest

RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y \
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
        curl \
        llvm-dev \
        libclang-dev \
        clang

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

WORKDIR /app/mishegos
COPY ./ .

ARG TARGET=all
RUN make "${TARGET}" -j

CMD ["/bin/bash"]

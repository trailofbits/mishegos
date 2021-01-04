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
        gcc-10

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

WORKDIR /app/mishegos
COPY ./ .

ARG TARGET=all
RUN make "${TARGET}" CC=gcc-10 CXX=g++-10 -j

CMD ["/bin/bash"]

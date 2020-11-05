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
        git

WORKDIR /app/mishegos
COPY ./ .

ARG TARGET=all
ARG BUILD_JOBS=4
RUN make "${TARGET}" -j"${BUILD_JOBS}"

CMD ["/bin/bash"]

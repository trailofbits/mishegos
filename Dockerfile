FROM ubuntu:latest

RUN apt-key adv --keyserver hkp://pool.sks-keyservers.net --recv 379CE192D401AB61 && \
    echo "deb https://dl.bintray.com/kaitai-io/debian jessie main" \
        | tee /etc/apt/sources.list.d/kaitai.list && \
    apt-get update && \
    apt-get install -y \
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
        kaitai-struct-compiler

WORKDIR /app/mishegos
COPY ./ .

ARG TARGET=all
ARG BUILD_JOBS=4
RUN make "${TARGET}" -j"${BUILD_JOBS}"

CMD ["/bin/bash"]

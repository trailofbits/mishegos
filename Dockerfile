FROM ubuntu:latest

RUN apt-get update && \
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
        gnupg2 \
        default-jre \
        python3-pip && \
    apt-key adv --keyserver hkp://keyserver.ubuntu.com --recv 379CE192D401AB61 && \
    echo "deb https://dl.bintray.com/kaitai-io/debian jessie main" \
        | tee /etc/apt/sources.list.d/kaitai.list && \
    apt-get update && \
    apt-get install -y \
        kaitai-struct-compiler && \
    pip3 install kaitaistruct

WORKDIR /app/mishegos
COPY ./ .

ARG TARGET=all
ARG BUILD_JOBS=4
RUN make "${TARGET}" -j"${BUILD_JOBS}"

CMD ["/bin/bash"]

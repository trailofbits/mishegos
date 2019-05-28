FROM ubuntu:latest

RUN apt update && apt install -y build-essential binutils-dev python

WORKDIR /app/mishegos
COPY ./ .

ARG TARGET=all
ARG BUILD_JOBS=4
RUN make "${TARGET}" -j"${BUILD_JOBS}"

CMD ["/bin/bash"]

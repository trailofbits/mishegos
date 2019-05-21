FROM ubuntu:latest

RUN apt update && apt install -y build-essential binutils-dev

WORKDIR /app/mishegos
COPY ./ .

RUN make debug

CMD ["/bin/bash"]

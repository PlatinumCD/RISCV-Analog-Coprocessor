FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

ENV RACS_SRC=/src
ENV RACS_BUILD=/opt
ENV NUM_THREADS=64

RUN mkdir -p $RACS_SRC
RUN mkdir -p $RACS_BUILD

RUN apt-get update
RUN apt-get install -y git

RUN git clone https://github.com/riscv/riscv-gnu-toolchain $RACS_SRC/riscv
WORKDIR $RACS_SRC/riscv

RUN apt-get install -y build-essential
RUN apt-get install -y gawk
RUN apt-get install -y curl

RUN ./configure --prefix=$RACS_BUILD/riscv --enable-multilib
RUN make -j ${NUM_THREADS}
WORKDIR /

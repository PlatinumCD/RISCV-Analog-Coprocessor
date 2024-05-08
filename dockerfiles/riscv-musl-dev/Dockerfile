FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# Build Args
ARG BUILD_SRC
ARG BUILD_DEST
ARG NUM_THREADS

# RISCV Args
ARG RISCV_GIT
ARG RISCV_BRANCH

# Install Dependencies
RUN apt-get update -y && \
    apt-get install -y autoconf automake autotools-dev curl \
            python3 python3-pip libmpc-dev libmpfr-dev libgmp-dev \
            gawk build-essential bison flex texinfo gperf libtool \
            patchutils bc zlib1g-dev libexpat-dev ninja-build git \
            cmake libglib2.0-dev libslirp-dev

# RISC-V
RUN git clone -b ${RISCV_BRANCH} ${RISCV_GIT} ${BUILD_SRC}/riscv
ADD ./gitmodules ${BUILD_SRC}/riscv/.gitmodules
WORKDIR ${BUILD_SRC}/riscv
RUN ./configure --prefix=${BUILD_DEST}/riscv
RUN make -j ${NUM_THREADS} musl
WORKDIR /

# Add RISC-V binaries to $PATH
ENV PATH="${PATH}:${BUILD_DEST}/riscv/bin"

FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# Arguments
ENV RACS_SRC=/src
ENV RACS_BUILD=/opt
ENV NUM_THREADS=64

# Source and Build directories
RUN mkdir -p $RACS_SRC
RUN mkdir -p $RACS_BUILD

# Dependencies
RUN apt-get update 
RUN apt-get install -y autoconf automake autotools-dev curl \
                       python3 python3-pip libmpc-dev libmpfr-dev \
                       libgmp-dev gawk build-essential bison flex \
                       texinfo gperf libtool patchutils bc zlib1g-dev \
                       libexpat-dev ninja-build git cmake libglib2.0-dev

# RISC-V
RUN git clone https://github.com/riscv/riscv-gnu-toolchain $RACS_SRC/riscv
WORKDIR $RACS_SRC/riscv
RUN ./configure --prefix=$RACS_BUILD/riscv --enable-multilib
RUN make -j ${NUM_THREADS}
WORKDIR /

# LLVM
RUN git clone -b llvm-riscv https://github.com/PlatinumCD/llvm-project.git $RACS_SRC/llvm-project
RUN mkdir $RACS_SRC/llvm-project/build
WORKDIR $RACS_SRC/llvm-project/build
RUN cmake -S ../llvm -G Ninja -DCMAKE_INSTALL_PREFIX="${RACS_BUILD}/llvm" \
        -DCMAKE_BUILD_TYPE=Debug  -DLLVM_ENABLE_PROJECTS=clang \
        -DBUILD_SHARED_LIBS=True -DLLVM_OPTIMIZED_TABLEGEN=ON \
        -DLLVM_BUILD_TESTS=False \
        -DDEFAULT_SYSROOT="${RACS_BUILD}/riscv/riscv64-unknown-elf" \
        -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-elf" \
        -DLLVM_TARGETS_TO_BUILD="RISCV"
RUN cmake --build . --target install -- -j ${NUM_THREADS}
WORKDIR /

# Add llvm binaries to $PATH
ENV PATH="${PATH}:${RACS_BUILD}/llvm/bin"

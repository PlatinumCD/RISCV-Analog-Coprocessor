# Image Args
ARG DOCKERHUB_USER
ARG RELEASE
ARG RISCV_MUSL_IMAGE_NAME

FROM ${DOCKERHUB_USER}/${RISCV_MUSL_IMAGE_NAME}:${RELEASE} AS builder
ENV DEBIAN_FRONTEND=noninteractive

# Build Args
ARG BUILD_SRC
ARG BUILD_DEST
ARG NUM_THREADS

# LLVM Args
ARG LLVM_GIT
ARG LLVM_BRANCH

# Install Dependencies
RUN apt-get update -y && \
    apt-get install -y ca-certificates gnupg build-essential \
                       cmake make python3 zlib1g wget subversion \
                       unzip git ninja-build && \
    rm -rf /var/lib/apt/lists/*

# LLVM
RUN git clone -b ${LLVM_BRANCH} ${LLVM_GIT} ${BUILD_SRC}/llvm-project

# Build LLVM and Clang as a cross-compiler
RUN mkdir ${BUILD_SRC}/llvm-project/build && \
    cd ${BUILD_SRC}/llvm-project/build && \
    cmake -S ../llvm -G Ninja \
        -DCMAKE_INSTALL_PREFIX="${BUILD_DEST}/llvm"  \
        -DCMAKE_BUILD_TYPE=Release  \
        -DLLVM_ENABLE_PROJECTS="clang" \
        -DBUILD_SHARED_LIBS=False \
        -DLLVM_OPTIMIZED_TABLEGEN=ON \
        -DLLVM_BUILD_TESTS=False \
        -DLLVM_TARGETS_TO_BUILD="RISCV" \
        -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-linux-musl" && \
    cmake --build . --target install -- -j ${NUM_THREADS}

# Cross-compile the OpenMP runtime library
ENV BUILD_DEST="${BUILD_DEST}"
ADD ./analog_riscv_toolchain.cmake ${BUILD_SRC}/analog_riscv_toolchain.cmake
RUN mkdir -p ${BUILD_SRC}/llvm-project/openmp/build && \
    cd ${BUILD_SRC}/llvm-project/openmp/build && \
    cmake -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=${BUILD_SRC}/analog_riscv_toolchain.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=${BUILD_DEST}/riscv/sysroot/usr \
        -DLIBOMP_ENABLE_SHARED=OFF \
        -DLIBOMP_ARCH="riscv64" \
        -DLIBOMP_CROSS_COMPILING=ON \
        -DOPENMP_ENABLE_LIBOMPTARGET=OFF \
        -DLIBOMP_OMPT_SUPPORT=OFF \
        -DENABLE_OMPT_TOOLS=OFF \
        ../ && \
    cmake --build . --target install -- -j ${NUM_THREADS}

# Copy build to new image
FROM ubuntu:22.04
ARG BUILD_DEST
COPY --from=builder ${BUILD_DEST} ${BUILD_DEST}

# Add LLVM binaries to $PATH
ENV BUILD_DEST="${BUILD_DEST}"
ENV PATH="${PATH}:${BUILD_DEST}/riscv/bin"
ENV PATH="${PATH}:${BUILD_DEST}/llvm/bin"

# Add tests
ADD ./tests ${BUILD_DEST}/llvm/tests

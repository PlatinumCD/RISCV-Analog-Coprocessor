# Image Args
ARG DOCKERHUB_USER
ARG RELEASE
ARG RISCV_MUSL_IMAGE_NAME

FROM ${DOCKERHUB_USER}/${RISCV_MUSL_IMAGE_NAME}:${RELEASE} as builder
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
RUN git clone -b ${LLVM_BRANCH} ${LLVM_GIT} ${BUILD_SRC}/llvm-project && \
    mkdir ${BUILD_SRC}/llvm-project/build && \
    cd $BUILD_SRC/llvm-project/build && \
    cmake -S ../llvm -G Ninja -DCMAKE_INSTALL_PREFIX="${BUILD_DEST}/llvm"  \
        -DCMAKE_BUILD_TYPE=Release  -DLLVM_ENABLE_PROJECTS=clang           \
        -DBUILD_SHARED_LIBS=True -DLLVM_OPTIMIZED_TABLEGEN=ON              \
        -DLLVM_BUILD_TESTS=False -DLLVM_TARGETS_TO_BUILD="RISCV"           \
        -DDEFAULT_SYSROOT="${BUILD_DEST}/riscv/sysroot"                    \
        -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-linux-musl" && \
    cmake --build . --target install -- -j ${NUM_THREADS} && \
    rm -rf ${BUILD_SRC}/llvm-project

# Copy build to new image
FROM ubuntu:22.04
COPY --from=builder ${BUILD_DEST} ${BUILD_DEST}

# Add LLVM binaries to $PATH
ARG BUILD_DEST
ENV PATH="${PATH}:${BUILD_DEST}/riscv/bin"
ENV PATH="${PATH}:${BUILD_DEST}/llvm/bin"

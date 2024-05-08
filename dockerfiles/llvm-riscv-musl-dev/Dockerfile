# Image Args
ARG DOCKERHUB_USER
ARG RELEASE
ARG RISCV_IMAGE_NAME

FROM ${DOCKERHUB_USER}/${RISCV_IMAGE_NAME}:${RELEASE}
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
    cmake make python3 zlib1g wget subversion unzip git ninja-build

## LLVM
RUN git clone -b ${LLVM_BRANCH} ${LLVM_GIT} ${BUILD_SRC}/llvm-project
RUN mkdir ${BUILD_SRC}/llvm-project/build
WORKDIR $BUILD_SRC/llvm-project/build
RUN cmake -S ../llvm -G Ninja -DCMAKE_INSTALL_PREFIX="${BUILD_DEST}/llvm"  \
        -DCMAKE_BUILD_TYPE=Release  -DLLVM_ENABLE_PROJECTS=clang     \
        -DBUILD_SHARED_LIBS=True -DLLVM_OPTIMIZED_TABLEGEN=ON              \
        -DLLVM_BUILD_TESTS=False -DLLVM_TARGETS_TO_BUILD="RISCV"           \
        -DDEFAULT_SYSROOT="${BUILD_DEST}/riscv/riscv64-unknown-linux-musl" \
        -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-linux-musl"
RUN cmake --build . --target install -- -j ${NUM_THREADS}
WORKDIR /
#
# Add LLVM binaries to $PATH
ENV PATH="${PATH}:${BUILD_DEST}/llvm/bin"

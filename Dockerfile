FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

ARG BUILD_SRC
ARG BUILD_DEST
ARG NUM_THREADS

ENV RISCV_GIT=https://github.com/riscv-collab/riscv-gnu-toolchain.git
ENV RISCV_BRANCH=master

ENV LLVM_GIT=https://github.com/PlatinumCD/llvm-project.git
ENV LLVM_BRANCH=llvm-riscv

ENV TORCH_MLIR_GIT=https://github.com/PlatinumCD/torch-mlir.git
ENV TORCH_MLIR_BRANCH=analog-dialect

ENV SST_CORE_GIT=https://github.com/sstsimulator/sst-core.git
ENV SST_CORE_BRANCH=v15.0.0_Final

ENV SST_ELEMENTS_GIT=https://github.com/sstsimulator/sst-elements.git
ENV SST_ELEMENTS_BRANCH=v15.0.0_Final

#──────────── system deps ────────────#
RUN apt-get update -y && \
    apt-get install -y --no-install-recommends \
        autoconf automake bc bison build-essential ca-certificates ccache clang \
        cmake curl flex gawk git gperf libexpat-dev libglib2.0-dev libgmp-dev \
        libmpc-dev libmpfr-dev libtool libtool-bin ninja-build patchutils python3.11 \
        python3.11-venv python3.11-dev python3.11-distutils python3-pip libopenmpi-dev openmpi-bin \
        texinfo unzip vim wget zlib1g-dev && \
    rm -rf /var/lib/apt/lists/*

COPY utils /utils

#──────────── RISC-V toolchain ────────────#
RUN git clone -b ${RISCV_BRANCH} --single-branch ${RISCV_GIT} ${BUILD_SRC}/riscv-gnu-toolchain && \
    cd ${BUILD_SRC}/riscv-gnu-toolchain && \
    mv /utils/riscv-gnu-toolchain/gitmodules .gitmodules && \
    ./configure --prefix=${BUILD_DEST}/riscv-gnu-toolchain --enable-threads=posix && \
    make -j${NUM_THREADS} musl

#──────────── Python venv ────────────#
RUN python3.11 -m venv ${BUILD_DEST}/mlir_venv && \
    . ${BUILD_DEST}/mlir_venv/bin/activate && \
    python -m pip install --no-cache-dir -r /utils/pyvenv/requirements.txt

#──────────── LLVM / MLIR / Clang ────────────#
RUN git clone -b ${LLVM_BRANCH} --single-branch ${LLVM_GIT} ${BUILD_SRC}/llvm-project

RUN mkdir ${BUILD_SRC}/llvm-project/build && \
    cd ${BUILD_SRC}/llvm-project/build && \
    . ${BUILD_DEST}/mlir_venv/bin/activate && \
    cmake -G Ninja -S ../llvm \
        -DCMAKE_INSTALL_PREFIX=${BUILD_DEST}/llvm-project \
        -DLLVM_ENABLE_PROJECTS="clang;mlir" \
        -DLLVM_TARGETS_TO_BUILD="RISCV" \
        -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=ON \
        -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DMLIR_ENABLE_EXECUTION_ENGINE=ON \
        -DLLVM_INSTALL_UTILS=ON \
        -DBUILD_SHARED_LIBS=OFF \
        -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-linux-musl" \
        -DMLIR_ENABLE_BINDINGS_PYTHON=ON \
        -DPython3_EXECUTABLE=$(which python3.11) && \
    cmake --build . --target install -j${NUM_THREADS}

#──────────── OpenMP runtime ────────────#
RUN mkdir -p ${BUILD_SRC}/llvm-project/openmp/build && \
    cd ${BUILD_SRC}/llvm-project/openmp/build && \
    cmake -G Ninja -S .. \
        -DCMAKE_TOOLCHAIN_FILE=/utils/llvm-omp/llvm_omp_toolchain.cmake \
        -DCMAKE_INSTALL_PREFIX=${BUILD_DEST}/riscv-gnu-toolchain/sysroot/usr \
        -DCMAKE_BUILD_TYPE=Release \
        -DLIBOMP_ENABLE_SHARED=OFF \
        -DLIBOMP_ARCH="riscv64" -DLIBOMP_CROSS_COMPILING=ON \
        -DOPENMP_ENABLE_LIBOMPTARGET=OFF && \
    cmake --build . --target install -j${NUM_THREADS}


#──────────── Torch-MLIR ────────────#
RUN git clone -b ${TORCH_MLIR_BRANCH}  --single-branch ${TORCH_MLIR_GIT} ${BUILD_SRC}/torch-mlir

ENV PATH=${PATH}:${BUILD_DEST}/llvm-project/bin/
RUN mkdir ${BUILD_SRC}/torch-mlir/build && \
    cd ${BUILD_SRC}/torch-mlir/build && \
    . ${BUILD_DEST}/mlir_venv/bin/activate && \
    cmake -G Ninja -S .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=${BUILD_DEST}/torch-mlir \
        -DMLIR_DIR=${BUILD_DEST}/llvm-project/lib/cmake/mlir \
        -DLLVM_DIR=${BUILD_DEST}/llvm-project/lib/cmake/llvm \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DTORCH_MLIR_ENABLE_STABLEHLO=OFF \
        -DBUILD_SHARED_LIBS=OFF \
        -DPython3_FIND_VIRTUALENV=ONLY \
        -DMLIR_ENABLE_BINDINGS_PYTHON=ON \
        -DLLVM_ENABLE_ASSERTIONS=ON \
        -DPython3_EXECUTABLE=$(which python3.11) && \
    cmake --build . --target install -j${NUM_THREADS}

RUN cp ${BUILD_SRC}/torch-mlir/build/python_packages/torch_mlir/torch_mlir/_mlir_libs/_jit_ir_importer* ${BUILD_DEST}/torch-mlir/python_packages/torch_mlir/torch_mlir/_mlir_libs/.

#──────────── SST-Core ────────────#
RUN git clone -b ${SST_CORE_BRANCH} --single-branch ${SST_CORE_GIT} ${BUILD_SRC}/sst-core
RUN cd ${BUILD_SRC}/sst-core && ./autogen.sh && \
    mkdir build && cd build && \
    ../configure MPICC=/bin/mpicc MPICXX=/bin/mpic++ --prefix=${BUILD_DEST}/sst-core && \
    make -j ${NUM_THREADS} install

#──────────── SST-Elements ────────────#
ENV PATH=${PATH}:${BUILD_DEST}/sst-core/bin
RUN git clone -b ${SST_ELEMENTS_BRANCH} --single-branch ${SST_ELEMENTS_GIT} ${BUILD_SRC}/sst-elements
RUN cd ${BUILD_SRC}/sst-elements && ./autogen.sh && \
    mkdir build && cd build && \
    ../configure MPICC=/bin/mpicc MPICXX=/bin/mpic++ --prefix=${BUILD_DEST}/sst-elements && \
    make -j ${NUM_THREADS} install

#!/bin/bash

ROOT_DIR=$(pwd)
BUILD_SRC=$ROOT_DIR/external_projects
BUILD_DEST=$ROOT_DIR/build
NUM_THREADS=$(nproc)
mkdir -p $BUILD_DEST

set -e


#──────────── Python venv ────────────#
python3.11 -m venv $BUILD_DEST/mlir_venv
source $BUILD_DEST/mlir_venv/bin/activate
python -m pip install --no-cache-dir -r $ROOT_DIR/utils/pyvenv/requirements.txt



#──────────── RISC-V toolchain ────────────#
RISCV_TOOLCHAIN_SRC=$BUILD_SRC/riscv-gnu-toolchain
RISCV_TOOLCHAIN_DEST=$BUILD_DEST/riscv-gnu-toolchain

if [ -d $RISCV_TOOLCHAIN_SRC ]; then
    pushd $RISCV_TOOLCHAIN_SRC
        cp $ROOT_DIR/utils/riscv-gnu-toolchain/gitmodules .gitmodules
        ./configure --prefix=$RISCV_TOOLCHAIN_DEST --enable-threads=posix
        make -j$NUM_THREADS musl
    popd
fi



#──────────── LLVM / MLIR / Clang ────────────#
LLVM_PROJECT_SRC=$BUILD_SRC/llvm-project
LLVM_PROJECT_DEST=$BUILD_DEST/llvm-project

if [ -d $LLVM_PROJECT_SRC ]; then
    pushd $LLVM_PROJECT_SRC
        mkdir -p build && cd build
        cmake -G Ninja -S ../llvm \
            -DCMAKE_INSTALL_PREFIX=$LLVM_PROJECT_DEST \
            -DLLVM_ENABLE_PROJECTS="clang;mlir" \
            -DLLVM_TARGETS_TO_BUILD="RISCV" \
            -DCMAKE_BUILD_TYPE=Release \
            -DLLVM_ENABLE_ASSERTIONS=ON \
            -DCMAKE_C_COMPILER=clang \
            -DCMAKE_CXX_COMPILER=clang++ \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
            -DMLIR_ENABLE_EXECUTION_ENGINE=ON \
            -DLLVM_INSTALL_UTILS=ON \
            -DBUILD_SHARED_LIBS=OFF \
            -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-linux-musl" \
            -DMLIR_ENABLE_BINDINGS_PYTHON=ON \
            -DPython3_EXECUTABLE=$(which python3.11)
        cmake --build . --target install -j$NUM_THREADS
    popd
fi



#──────────── OpenMP runtime ────────────#
LLVM_OMP_SRC=$LLVM_PROJECT_SRC/openmp

if [ -d $LLVM_PROJECT_SRC ] && [ -d $RISCV_TOOLCHAIN_DEST ]; then
    pushd $LLVM_OMP_SRC
        cmake -G Ninja -S .. \
            -DCMAKE_TOOLCHAIN_FILE=$ROOT_DIR/utils/llvm-omp/llvm_omp_toolchain.cmake \
            -DCMAKE_INSTALL_PREFIX=$RISCV_TOOLCHAIN_DEST/sysroot/usr \
            -DCMAKE_BUILD_TYPE=Release \
            -DLIBOMP_ENABLE_SHARED=OFF \
            -DLIBOMP_ARCH="riscv64" \
            -DLIBOMP_CROSS_COMPILING=ON \
            -DOPENMP_ENABLE_LIBOMPTARGET=OFF
        cmake --build . --target install -j$NUM_THREADS
    popd
fi




#──────────── Torch-MLIR ────────────#
TORCH_MLIR_SRC=$BUILD_SRC/torch-mlir
TORCH_MLIR_DEST=$BUILD_DEST/torch_mlir

if [ -d $TORCH_MLIR_SRC ]; then
    pushd $TORCH_MLIR_DEST
        cmake -G Ninja -S .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=$TORCH_MLIR_DEST \
            -DMLIR_DIR=$LLVM_PROJECT_DEST/lib/cmake/mlir \
            -DLLVM_DIR=$LLVM_PROJECT_DEST/lib/cmake/llvm \
            -DCMAKE_C_COMPILER=clang \
            -DCMAKE_CXX_COMPILER=clang++ \
            -DCMAKE_C_COMPILER_LAUNCHER=ccache \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
            -DTORCH_MLIR_ENABLE_STABLEHLO=OFF \
            -DTORCH_MLIR_INCLUDE_TESTS=OFF \
            -DBUILD_SHARED_LIBS=OFF \
            -DPython3_FIND_VIRTUALENV=ONLY \
            -DMLIR_ENABLE_BINDINGS_PYTHON=ON \
            -DLLVM_ENABLE_ASSERTIONS=ON \
            -DPython3_EXECUTABLE=$(which python3.11)
        cmake --build . --target install -j$NUM_THREADS
    popd

    cp $TORCH_MLIR_BUILD/python_packages/torch_mlir/torch_mlir/_mlir_libs/_jit_ir_importer* \
       $BUILD_DEST/torch-mlir/python_packages/torch_mlir/torch_mlir/_mlir_libs/.
fi




#──────────── SST-Core ────────────#
SST_CORE_SRC=$BUILD_SRC/sst-core
SST_CORE_DEST=$BUILD_DEST/sst-core

if [ -d $SST_CORE_SRC ]; then
    pushd $SST_CORE_SRC
        ./autogen.sh
        mkdir -p build && cd build
        ../configure MPICC=/bin/mpicc MPICXX=/bin/mpic++ --prefix=$SST_CORE_DEST
        make -j$NUM_THREADS install
    popd
fi


#──────────── SST-Elements ────────────#
SST_ELEMENTS_SRC=$BUILD_SRC/sst-elements
SST_ELEMENTS_DEST=$BUILD_DEST/sst-elements

export PATH=$SST_CORE_DEST/bin:$PATH

if [ -d $SST_ELEMENTS_SRC ] && [ -d $SST_CORE_DEST ]; then
    pushd $SST_ELEMENTS_SRC
        ./autogen.sh
        mkdir -p build && cd build
        ../configure MPICC=/bin/mpicc MPICXX=/bin/mpic++ --prefix=$SST_ELEMENTS_DEST
        make -j$NUM_THREADS install
    popd
fi

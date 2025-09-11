#!/usr/bin/env bash
set -eo pipefail

# ================= Compiler ================= #
export ROOT="$(realpath "$(pwd)"/..)"
export BUILD_DEST="$ROOT/build"
COMPILER="$BUILD_DEST/llvm-project/bin/clang++"
TARGET="riscv64-unknown-linux-musl"
TOOLCHAIN="$BUILD_DEST/riscv-gnu-toolchain"
SYSROOT="$TOOLCHAIN/sysroot"

RCC="riscv64-unknown-linux-musl-g++"
RCXX_FLAGS="-static -fopenmp"

CC="$COMPILER --target=$TARGET --gcc-toolchain=$TOOLCHAIN --sysroot=$SYSROOT"
CXX_FLAGS="-static" 


# ================= SST Setup ================ #
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DEST/python3.11/lib:$BUILD_DEST/isl/lib"
export PATH="$PATH:$BUILD_DEST/python3.11/bin:$BUILD_DEST/llvm-project/bin:$BUILD_DEST/sst-core/bin"
export PATH="$PATH:$TOOLCHAIN/bin"
module load openmpi


# ================= Params =================== #
export GOLEM_ARRAY_TYPE="golem.CrossSimFloatArray"

export VANADIS_NUM_CORES=64
export GOLEM_NUM_ARRAYS=1
export VANADIS_EXE=$(pwd)/test.exe


export ARRAY_INPUT_SIZE=2
export ARRAY_OUTPUT_SIZE=2


echo "Trial: Test"
echo "  Num arrays: $GOLEM_NUM_ARRAYS"
echo "  Num coress: $VANADIS_NUM_CORES"
echo "  Size: ${ARRAY_INPUT_SIZE}x${ARRAY_OUTPUT_SIZE}"
echo "  Source: $CPP_FILE"
echo "  Target: $TARGET_EXE"

$CC $CXX_FLAGS -c kernel.cpp -o kernel.o
$RCC $RCXX_FLAGS \
  -DINPUT_SIZE="$INPUT_SIZE" \
  -DNUM_ARRAYS="$GOLEM_NUM_ARRAYS" \
  -DNUM_CORES="$VANADIS_NUM_CORES" \
  test.cpp kernel.o -o test.exe

sst sst-runtime-configuration.py

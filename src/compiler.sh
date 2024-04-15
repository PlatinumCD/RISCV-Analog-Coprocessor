#!/bin/bash

RACS_BUILD=/opt

CC=clang++
SYSROOT=$RACS_BUILD/riscv/riscv64-unknown-elf
GCC_TOOLCHAIN=$RACS_BUILD/riscv
TARGET=riscv64
ARCH=rv64g

docker run               \
    --rm -v              \
    $(pwd):/src          \
    platinumcd/llvm-riscv-toolchain \
    $CC                  \
        --sysroot=$SYSROOT             \
        --gcc-toolchain=$GCC_TOOLCHAIN \
        --target=$TARGET               \
        -march=$ARCH                   \
        /src/$1 -o /src/"${1%.cpp}_cpp.out"

# Define paths
COMPILER=$BUILD_DEST/llvm/bin/clang++
TARGET=riscv64-unknown-linux-musl
TOOLCHAIN=$BUILD_DEST/riscv
SYSROOT=$BUILD_DEST/riscv/sysroot

# Define the full command using the variables
CC="$COMPILER --target=$TARGET --gcc-toolchain=$TOOLCHAIN --sysroot=$SYSROOT"

# Compile the OpenMP example
$CC -fopenmp openmp_example.cpp -o openmp_example

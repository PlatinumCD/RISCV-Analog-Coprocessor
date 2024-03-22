# RISC-V Analog Co-processor Simulator

This project is a simulation of a RISC-V analog co-processor. It leverages a variety of tools listed below to generate an accurate simulation. 

### Tools Used 
---

- [RISC-V GNU Toolchain](https://github.com/riscv/riscv-gnu-toolchain): The RISC-V GNU Compiler Collection provides a standard, open-source compiler for the RISC-V architecture.

- [LLVM](https://github.com/llvm/llvm-project): The LLVM Project is a collection of modular and reusable compiler and toolchain technologies.

- [SST Core](https://github.com/sstsimulator/sst-core): The Structural Simulation Toolkit (SST) Core provides a parallel discrete event simulation (PDES) framework for performing architecture simulations of existing and proposed HPC systems.

- [SST Elements](https://github.com/sstsimulator/sst-elements/): SST Elements house implementations of various HPC-component models, assembled into runtimes to simulate architectures. 

- [CrossSim](https://github.com/sandialabs/cross-sim): CrossSim is a cross-architecture instruction-level simulator with support for x86_64, ARM AArch64, and RISC-V instruction sets.

### Building the analog LLVM RISC-V Toolchain
---

To build the analog LLVM RISC-V Toolchain, the RISC-V GNU Toolchain must be built prior to building LLVM. The following code blocks show the commands for building the compiler. I will be using the variables `$RACS_SRC` to represent the source directory, `$RACS_BUILD` to represent the build directory, and `$NUM_THREADS` to represent the number of threads for `make` to use.

__RISC-V GNU Toolchain:__
```
git clone https://github.com/riscv/riscv-gnu-toolchain $RACS_SRC/riscv
cd $RACS_SRC/riscv
./configure --prefix=$RACS_BUILD/riscv --enable-multilib
make -j ${NUM_THREADS}
```

__LLVM RISC-V Toolchain:__
```
git clone -b llvm-riscv https://github.com/PlatinumCD/llvm-project.git $RACS_SRC/llvm-project
mkdir $RACS_SRC/llvm-project/build
cd $RACS_SRC/llvm-project/build
cmake -S ../llvm -G Ninja -DCMAKE_INSTALL_PREFIX="${RACS_BUILD}/llvm" \
        -DCMAKE_BUILD_TYPE=Debug  -DLLVM_ENABLE_PROJECTS=clang \
        -DBUILD_SHARED_LIBS=True -DLLVM_OPTIMIZED_TABLEGEN=ON \
        -DLLVM_BUILD_TESTS=False \
        -DDEFAULT_SYSROOT="${RACS_BUILD}/riscv/riscv64-unknown-elf" \
        -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-elf" \
        -DLLVM_TARGETS_TO_BUILD="RISCV"
cmake --build . --target install -- -j ${NUM_THREADS}
```

The installation may take some time. After the installation of the the LLVM RISC-V Toolchain, add the LLVM binaries to your path with:
```
export PATH="${PATH}:${RACS_BUILD}/llvm/bin"
```



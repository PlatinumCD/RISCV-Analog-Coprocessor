# RISC-V Analog Co-processor Simulator

This project contains the source code to simulate a RISC-V analog co-processor. The project uses the following tools:

- [RISC-V GNU Toolchain](https://github.com/riscv/riscv-gnu-toolchain): The RISC-V GNU Toolchain is required to build RISC-V binaries. The toolchain is important for providing the root directory for headers, libraries etc. when cross compiling with LLVM.

- [Custom LLVM Build](https://github.com/PlatinumCD/llvm-project/tree/llvm-riscv): This modified version of the LLVM Project is required to build RISC-V analog binaries. It contains an ISA extension to support analog operations. It uses the RISC-V system root.

- [SST Core](https://github.com/sstsimulator/sst-core): SST Core is responsible for providing an interface that supports our custom SST Elements. 

- [Custom SST Elements](https://github.com/PlatinumCD/sst-elements/tree/basic_rocc): This modified version of SST Elements contains the Vanadis element that supports the LLVM ISA extension.  

- [CrossSim](https://github.com/sandialabs/cross-sim): CrossSim simulates the analog computation. 

## Install

This project can be broken down into two critical components:

- The Analog LLVM RISC-V Toolchain: Used for compiling analog binaries.
- The CrossSim-integrated SST: Used for simulating the runtime of analog binaries.

I've provided the instructions to build these components. I've also provided Dockerfiles to build the components individually and together. The instructions are listed below:

### Building the Analog LLVM RISC-V Toolchain:
---

To build the Analog LLVM RISC-V Toolchain, the RISC-V GNU Toolchain must be built prior to building LLVM. The following code blocks show the commands for building the RISC-V GNU Toolchain as well as the Analog LLVM RISC-V Toolchain. I will be using the variables `$RACS_SRC` to represent the source directory, `$RACS_BUILD` to represent the build directory, and `$NUM_THREADS` to represent the number of threads for `make` to use. A Dockerfile to build the toolchain is included in the `dockerfiles` directory. 

__Source/Build/Threads Configuration:__
```
export RACS_SRC=/src
export RACS_BUILD=/opt
export NUM_THREADS=16
```

__RISC-V GNU Toolchain:__
```
git clone https://github.com/riscv/riscv-gnu-toolchain $RACS_SRC/riscv
cd $RACS_SRC/riscv
./configure --prefix=$RACS_BUILD/riscv --enable-multilib
make -j ${NUM_THREADS}
export PATH="${PATH}:${RACS_BUILD}/riscv/bin"
```

__LLVM RISC-V Toolchain:__
```
git clone -b llvm-riscv https://github.com/PlatinumCD/llvm-project.git $RACS_SRC/llvm-project
mkdir $RACS_SRC/llvm-project/build
cd $RACS_SRC/llvm-project/build
cmake -S ../llvm -G Ninja -DCMAKE_INSTALL_PREFIX="${RACS_BUILD}/llvm" \
    -DCMAKE_BUILD_TYPE=Debug -DLLVM_ENABLE_PROJECTS=clang         \
    -DBUILD_SHARED_LIBS=True -DLLVM_OPTIMIZED_TABLEGEN=ON         \
    -DLLVM_BUILD_TESTS=False -DLLVM_TARGETS_TO_BUILD="RISCV"      \
    -DDEFAULT_SYSROOT="${RACS_BUILD}/riscv/riscv64-unknown-elf"   \
    -DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-elf"            \
cmake --build . --target install -- -j ${NUM_THREADS}
```

The installation may take some time. After the installation of the the LLVM RISC-V Toolchain, add the LLVM binaries to your path with:
```
export PATH="${PATH}:${RACS_BUILD}/llvm/bin"
```

Check to make sure the binary was installed correctly:
```
$ clang -v
clang version 18.0.0 (https://github.com/PlatinumCD/llvm-project.git 24a617687a708fb31edf5e58b6dbaf2cdab2a578)
Target: riscv64-unknown-unknown-elf
Thread model: posix
InstalledDir: /opt/llvm/bin
```

### Building CrossSim-integrated SST

To build the CrossSim-integrated SST, we need to install the SST Core, custom SST Elements, and CrossSim. I will reuse the `$RACS_SRC`, `$RACS_BUILD`, `$NUM_THREADS` variables.

__Building SST Core:__
```
git clone https://github.com/sstsimulator/sst-core.git ${RACS_SRC}/sst-core
pip install blessings pygments
cd ${RACS_SRC}/sst-core && ./autogen.sh
mkdir ${RACS_SRC}/build && cd ${RACS_SRC}/build
../configure           \
    MPICC=/bin/mpicc   \
    MPICXX=/bin/mpic++ \
    --prefix=${RACS_BUILD}/sst-core
make -j ${NUM_THREADS} install
export PATH="${PATH}:${RACS_BUILD}/sst-core/bin"
```

__Building custom SST Elements:__
```
git clone https://github.com/PlatinumCD/sst-elements.git ${RACS_SRC}/sst-elements
cd ${RACS_SRC}/sst-elements && ./autogen.sh
mkdir ${RACS_SRC}/sst-elements/build && cd ${RACS_SRC}/sst-elements/build
../configure                                                              \
    MPICC=/bin/mpicc                                                      \
    MPICXX=/bin/mpic++                                                    \
    CPPFLAGS=-I/usr/local/lib/python3.10/dist-packages/numpy/core/include \
    --prefix=${RACS_BUILD}/sst-elements
make -j ${NUM_THREADS} install
```

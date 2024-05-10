# Release Args
export RELEASE=1.0.0
export PLATFORM=linux/amd64,linux/arm64
export DOCKERHUB_USER=platinumcd

# Build Args
export BUILD_SRC=/src
export BUILD_DEST=/opt
export NUM_THREADS=32

# SST-CrossSim
export SST_CROSSSIM_IMAGE_NAME=analog-sst-crosssim-dev
export CROSSSIM_GIT=https://github.com/sandialabs/cross-sim.git
export CROSSSIM_BRANCH=v3.0
export SSTCORE_GIT=https://github.com/sstsimulator/sst-core.git
export SSTCORE_BRANCH=master
export SSTELEMENTS_GIT=https://github.com/PlatinumCD/sst-elements.git
export SSTELEMENTS_BRANCH=basic_rocc

# RISC-V Musl compiler
export RISCV_MUSL_IMAGE_NAME=analog-riscv-musl-dev
export RISCV_GIT=https://github.com/riscv-collab/riscv-gnu-toolchain.git
export RISCV_BRANCH=master

# LLVM RISC-V Musl cross-compiler
export LLVM_RISCV_MUSL_IMAGE_NAME=analog-llvm-riscv-musl-dev
export LLVM_GIT=https://github.com/PlatinumCD/llvm-project.git
export LLVM_BRANCH=llvm-riscv

# Build type (image/push)
export MAKE_TARGET=image

pushd sst-crosssim
make $MAKE_TARGET
popd
pushd riscv-musl
make $MAKE_TARGET
popd
pushd llvm-riscv-musl
make $MAKE_TARGET
popd
pushd analog-stack
make $MAKE_TARGET
popd

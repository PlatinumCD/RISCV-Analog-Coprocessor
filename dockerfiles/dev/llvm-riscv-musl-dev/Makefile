# Release Args
RELEASE?=1.0.0
PLATFORM?=linux/amd64,linux/arm64
DOCKERHUB_USER?=platinumcd
LLVM_RISCV_MUSL_IMAGE_NAME?=analog-llvm-riscv-musl-dev

# Build Args
RISCV_MUSL_IMAGE_NAME?=analog-riscv-musl-dev
BUILD_SRC?=/src
BUILD_DEST?=/opt
NUM_THREADS?=32

# LLVM Args
LLVM_GIT?=https://github.com/PlatinumCD/llvm-project.git
LLVM_BRANCH?=llvm-riscv

all: image

image:
	docker build \
		--build-arg DOCKERHUB_USER=$(DOCKERHUB_USER) \
		--build-arg RISCV_MUSL_IMAGE_NAME=$(RISCV_MUSL_IMAGE_NAME) \
		--build-arg RELEASE=$(RELEASE) \
		--build-arg BUILD_SRC=$(BUILD_SRC) \
		--build-arg BUILD_DEST=$(BUILD_DEST) \
		--build-arg NUM_THREADS=$(NUM_THREADS) \
		--build-arg LLVM_GIT=$(LLVM_GIT) \
		--build-arg LLVM_BRANCH=$(LLVM_BRANCH) \
		-t "$(DOCKERHUB_USER)/$(LLVM_RISCV_MUSL_IMAGE_NAME):$(RELEASE)" \
		.

push:
	docker build \
		--build-arg DOCKERHUB_USER=$(DOCKERHUB_USER) \
		--build-arg RISCV_MUSL_IMAGE_NAME=$(RISCV_MUSL_IMAGE_NAME) \
		--build-arg RELEASE=$(RELEASE) \
		--build-arg BUILD_SRC=$(BUILD_SRC) \
		--build-arg BUILD_DEST=$(BUILD_DEST) \
		--build-arg NUM_THREADS=$(NUM_THREADS) \
		--build-arg LLVM_GIT=$(LLVM_GIT) \
		--build-arg LLVM_BRANCH=$(LLVM_BRANCH) \
		-t "$(DOCKERHUB_USER)/$(LLVM_RISCV_MUSL_IMAGE_NAME):$(RELEASE)" \
		--platform "$(PLATFORM)" --push \
		.

test:
	./cpp_compiler ./tests/hello_world.cpp
	./cpp_compiler ./tests/loop.cpp

clean-test:
	rm -rf ./tests/hello_world_cpp.out
	rm -rf ./tests/loop_cpp.out

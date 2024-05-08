# Release Args
RELEASE?=1.0.0
PLATFORM?=linux/amd64,linux/arm64
DOCKERHUB_USER?=platinumcd
IMAGE_NAME?=analog-llvm-riscv-musl-dev

# Build Args
RISCV_IMAGE_NAME?=analog-riscv-musl-dev
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
		--build-arg RISCV_IMAGE_NAME=$(RISCV_IMAGE_NAME) \
		--build-arg RELEASE=$(RELEASE) \
		--build-arg BUILD_SRC=$(BUILD_SRC) \
		--build-arg BUILD_DEST=$(BUILD_DEST) \
		--build-arg NUM_THREADS=$(NUM_THREADS) \
		--build-arg LLVM_GIT=$(LLVM_GIT) \
		--build-arg LLVM_BRANCH=$(LLVM_BRANCH) \
		-t "$(DOCKERHUB_USER)/$(IMAGE_NAME):$(RELEASE)" \
		.

push:
	docker build \
		--build-arg DOCKERHUB_USER=$(DOCKERHUB_USER) \
		--build-arg RISCV_IMAGE_NAME=$(RISCV_IMAGE_NAME) \
		--build-arg RELEASE=$(RELEASE) \
		--build-arg BUILD_SRC=$(BUILD_SRC) \
		--build-arg BUILD_DEST=$(BUILD_DEST) \
		--build-arg NUM_THREADS=$(NUM_THREADS) \
		--build-arg LLVM_GIT=$(LLVM_GIT) \
		--build-arg LLVM_BRANCH=$(LLVM_BRANCH) \
		-t "$(DOCKERHUB_USER)/$(IMAGE_NAME):$(RELEASE)" \
		--platform "$(PLATFORM)" --push \
		.

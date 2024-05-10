# Release Args
RELEASE?=1.0.0
PLATFORM?=linux/amd64,linux/arm64
DOCKERHUB_USER?=platinumcd
IMAGE_NAME?=analog-stack

# Build Args
LLVM_RISCV_IMAGE_NAME?=analog-llvm-riscv-musl
SST_CROSSSIM_IMAGE_NAME?=analog-sst-crosssim
BUILD_SRC?=/src
BUILD_DEST?=/opt

all: image

image:
	docker build \
		--build-arg DOCKERHUB_USER=$(DOCKERHUB_USER) \
		--build-arg RISCV_IMAGE_NAME=$(RISCV_IMAGE_NAME) \
		--build-arg RELEASE=$(RELEASE) \
		--build-arg BUILD_SRC=$(BUILD_SRC) \
		--build-arg BUILD_DEST=$(BUILD_DEST) \
		--build-arg LLVM_RISCV_IMAGE_NAME=$(LLVM_RISCV_IMAGE_NAME) \
		--build-arg SST_CROSSSIM_IMAGE_NAME=$(SST_CROSSSIM_IMAGE_NAME) \
		-t "$(DOCKERHUB_USER)/$(IMAGE_NAME):$(RELEASE)" \
		.

push:
	docker build \
		--build-arg DOCKERHUB_USER=$(DOCKERHUB_USER) \
		--build-arg RISCV_IMAGE_NAME=$(RISCV_IMAGE_NAME) \
		--build-arg RELEASE=$(RELEASE) \
		--build-arg BUILD_SRC=$(BUILD_SRC) \
		--build-arg BUILD_DEST=$(BUILD_DEST) \
		--build-arg LLVM_RISCV_IMAGE_NAME=$(LLVM_RISCV_IMAGE_NAME) \
		--build-arg SST_CROSSSIM_IMAGE_NAME=$(SST_CROSSSIM_IMAGE_NAME) \
		-t "$(DOCKERHUB_USER)/$(IMAGE_NAME):$(RELEASE)" \
		--platform "$(PLATFORM)" --push \
		.

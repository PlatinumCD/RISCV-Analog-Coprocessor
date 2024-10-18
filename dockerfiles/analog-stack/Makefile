# Release Args
RELEASE?=1.0.0
PLATFORM?=linux/amd64,linux/arm64
DOCKERHUB_USER?=platinumcd
ANALOG_STACK_IMAGE_NAME?=analog-stack

ANALOG_LIB_GIT?=https://github.com/PlatinumCD/analog-library.git
ANALOG_LIB_BRANCH?=main

# Build Args
LLVM_RISCV_MUSL_IMAGE_NAME?=analog-llvm-riscv-musl
SST_CROSSSIM_IMAGE_NAME?=analog-sst-crosssim
BUILD_SRC?=/src
BUILD_DEST?=/opt

all: image

image:
	docker build \
		--build-arg DOCKERHUB_USER=$(DOCKERHUB_USER) \
		--build-arg RELEASE=$(RELEASE) \
		--build-arg BUILD_SRC=$(BUILD_SRC) \
		--build-arg BUILD_DEST=$(BUILD_DEST) \
		--build-arg LLVM_RISCV_MUSL_IMAGE_NAME=$(LLVM_RISCV_MUSL_IMAGE_NAME) \
		--build-arg SST_CROSSSIM_IMAGE_NAME=$(SST_CROSSSIM_IMAGE_NAME) \
		--build-arg ANALOG_LIB_GIT=$(ANALOG_LIB_GIT) \
		--build-arg ANALOG_LIB_BRANCH=$(ANALOG_LIB_BRANCH) \
		-t "$(DOCKERHUB_USER)/$(ANALOG_STACK_IMAGE_NAME):$(RELEASE)" \
		.

push:
	docker build \
		--build-arg DOCKERHUB_USER=$(DOCKERHUB_USER) \
		--build-arg RELEASE=$(RELEASE) \
		--build-arg BUILD_SRC=$(BUILD_SRC) \
		--build-arg BUILD_DEST=$(BUILD_DEST) \
		--build-arg LLVM_RISCV_MUSL_IMAGE_NAME=$(LLVM_RISCV_MUSL_IMAGE_NAME) \
		--build-arg SST_CROSSSIM_IMAGE_NAME=$(SST_CROSSSIM_IMAGE_NAME) \
		--build-arg ANALOG_LIB_GIT=$(ANALOG_LIB_GIT) \
		--build-arg ANALOG_LIB_BRANCH=$(ANALOG_LIB_BRANCH) \
		-t "$(DOCKERHUB_USER)/$(ANALOG_STACK_IMAGE_NAME):$(RELEASE)" \
		--platform "$(PLATFORM)" --push \
		.

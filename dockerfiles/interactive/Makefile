# Release Args
RELEASE?=1.0.0
PLATFORM?=linux/amd64,linux/arm64
DOCKERHUB_USER?=platinumcd
ANALOG_INTERACTIVE_STACK_IMAGE_NAME?=analog-interactive-stack-dev

# Build Args
LLVM_RISCV_MUSL_IMAGE_NAME?=analog-llvm-riscv-musl
SST_CROSSSIM_IMAGE_NAME?=analog-sst-crosssim-dev
BUILD_SRC?=/src
BUILD_DEST?=/opt

# Get the current working directory
PWD := $(shell pwd)

.PHONY: run

run:
	docker run -it \
		-v $(PWD):/local \
		$(DOCKERHUB_USER)/$(ANALOG_INTERACTIVE_STACK_IMAGE_NAME):$(RELEASE)

build:
	docker build \
		--build-arg DOCKERHUB_USER=$(DOCKERHUB_USER) \
		--build-arg RELEASE=$(RELEASE) \
		--build-arg BUILD_SRC=$(BUILD_SRC) \
		--build-arg BUILD_DEST=$(BUILD_DEST) \
		--build-arg LLVM_RISCV_MUSL_IMAGE_NAME=$(LLVM_RISCV_MUSL_IMAGE_NAME) \
		--build-arg SST_CROSSSIM_IMAGE_NAME=$(SST_CROSSSIM_IMAGE_NAME) \
		-t "$(DOCKERHUB_USER)/$(ANALOG_INTERACTIVE_STACK_IMAGE_NAME):$(RELEASE)" \
		.

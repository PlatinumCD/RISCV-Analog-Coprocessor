# Release Args
RELEASE=1.0.0
DOCKERHUB_USER=platinumcd
ANALOG_STACK_IMAGE_NAME=analog-stack-dev

# Build type (image/push)
MAKE_TARGET=image

.PHONY: build run

build:
	# Build SST-CrossSim
	pushd sst-crosssim-dev && make $(MAKE_TARGET) && popd
	
	# Build RISCV musl toolchain
	pushd riscv-musl-dev && make $(MAKE_TARGET) && popd
	
	# Build LLVM RISCV musl compiler
	pushd llvm-riscv-musl-dev && make $(MAKE_TARGET) && popd
	
	# Build SST-CrossSim + LLVM RISCV musl compiler
	pushd analog-stack-dev && make $(MAKE_TARGET) && popd

run:
	# Run the SST-CrossSim image (example)
	docker run -it --rm \
		-v $(shell pwd):/local \
		"$(DOCKERHUB_USER)/$(ANALOG_STACK_IMAGE_NAME):$(RELEASE)"

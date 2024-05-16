# Analog Stack Development Environment

This directory contains the environment setup for the Analog Stack development. It includes a `Dockerfile` and a `Makefile` to build and manage the Docker image.

## Makefile Arguments

### Release Args
- `RELEASE` (default: `1.0.0`): The release version of the Docker image.
- `PLATFORM` (default: `linux/amd64,linux/arm64`): The target platform(s) for the Docker image.
- `DOCKERHUB_USER` (default: `platinumcd`): The Docker Hub username to tag the image.
- `ANALOG_STACK_IMAGE_NAME` (default: `analog-stack-dev`): The name of the Docker image.

### Build Args
- `LLVM_RISCV_MUSL_IMAGE_NAME` (default: `analog-llvm-riscv-musl-dev`): Name of the LLVM RISCV Musl image.
- `SST_CROSSSIM_IMAGE_NAME` (default: `analog-sst-crosssim-dev`): Name of the SST CrossSim image.
- `BUILD_SRC` (default: `/src`): Source build directory.
- `BUILD_DEST` (default: `/opt`): Destination build directory.

## Makefile Targets

### `all`
This is the default target that builds the Docker image.

Usage:
```bash
make
```

### `image`
Builds the Docker image.

Usage:
```bash
make image
```

### `push`
Builds and pushes the Docker image to the registry for the specified platforms.

Usage:
```bash
make push
```

## Example Commands

Build the image:
```bash
make image
```

Build and push the image to Docker Hub:
```bash
make push
```
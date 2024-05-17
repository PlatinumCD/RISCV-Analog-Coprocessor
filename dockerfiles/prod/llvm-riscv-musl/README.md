# LLVM RISC-V MUSL Production Environment

This directory contains the environment setup for the LLVM RISC-V MUSL production deployment. It includes a `Dockerfile` and a `Makefile` to build and manage the Docker image.

## Makefile Arguments

### Release Args
- `RELEASE` (default: `1.0.0`): The release version of the Docker image.
- `PLATFORM` (default: `linux/amd64,linux/arm64`): The target platform(s) for the Docker image.
- `DOCKERHUB_USER` (default: `platinumcd`): The Docker Hub username to tag the image.
- `LLVM_RISCV_MUSL_IMAGE_NAME` (default: `analog-llvm-riscv-musl`): The name of the Docker image.

### Build Args
- `RISCV_MUSL_IMAGE_NAME` (default: `analog-riscv-musl`): Name of the RISCV Musl image.
- `BUILD_SRC` (default: `/src`): Source build directory.
- `BUILD_DEST` (default: `/opt`): Destination build directory.
- `NUM_THREADS` (default: `32`): Number of threads to use for building.

### LLVM Args
- `LLVM_GIT` (default: `https://github.com/PlatinumCD/llvm-project.git`): Git repository for LLVM.
- `LLVM_BRANCH` (default: `llvm-riscv`): Git branch for LLVM.

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

### `test`
Runs tests using the built image.

Usage:
```bash
make test
```

### `clean-test`
Cleans up test outputs.

Usage:
```bash
make clean-test
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

Run the tests:
```bash
make test
```

Clean the test outputs:
```bash
make clean-test
```
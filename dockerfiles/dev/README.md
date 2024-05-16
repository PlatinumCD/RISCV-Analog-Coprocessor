# Project Repository

This project contains several development environments for different purposes. Each directory contains a `Dockerfile` and a `Makefile` to build the respective Docker images. Below is a description of each directory, the arguments used in their respective Makefiles, and how to build the images using these Makefiles.

## Directories Overview
1. `analog-stack-dev`
2. `llvm-riscv-musl-dev`
3. `riscv-musl-dev`
4. `sst-crosssim-dev`

Each directory has specific build arguments and targets defined in the Makefile. Detailed descriptions are provided in their respective README files.

## Building Docker Images

To build a Docker image, navigate to the directory and use the `make` command with the desired target:

```bash
cd <directory_name>
make <target>
```

By default, running `make` without a target will execute the `all` target, which builds the image.

Sample Usage:

```bash
cd analog-stack-dev
make image
```

## Pushing Docker Images

Some directories support pushing built images to a Docker registry:

```bash
make push
```

Sample Usage:

```bash
cd llvm-riscv-musl-dev
make push
```

For detailed build arguments and other options, refer to the README files in each directory.

## Automated Build Script

The `build_dev.sh` script in the root directory automates the build process for all developer images. It allows you to set global environment variables for the build process.

### Global Environment Variables

- `RELEASE`: The release version of the Docker images. Default is `1.0.0`.
- `PLATFORM`: The target platform(s) for the Docker images. Default is `linux/amd64,linux/arm64`.
- `DOCKERHUB_USER`: The Docker Hub username to tag the images.
- `BUILD_SRC`: Source build directory. Default is `/src`.
- `BUILD_DEST`: Destination build directory. Default is `/opt`.
- `NUM_THREADS`: Number of threads to use for building. Default is `32`.

### Specific Image Arguments

- **SST-CrossSim:**
  - `SST_CROSSSIM_IMAGE_NAME`
  - `CROSSSIM_GIT`
  - `CROSSSIM_BRANCH`
  - `SSTCORE_GIT`
  - `SSTCORE_BRANCH`
  - `SSTELEMENTS_GIT`
  - `SSTELEMENTS_BRANCH`
- **RISC-V Musl Compiler:**
  - `RISCV_MUSL_IMAGE_NAME`
  - `RISCV_GIT`
  - `RISCV_BRANCH`
- **LLVM RISC-V Musl Cross-compiler:**
  - `LLVM_RISCV_MUSL_IMAGE_NAME`
  - `LLVM_GIT`
  - `LLVM_BRANCH`
- **Build Type:** Can be `image` or `push`.

### Usage

To run the script, use:

```bash
./build_dev.sh
```

This script will navigate to each directory, set the appropriate build arguments, and execute the specified target (`image` or `push`).

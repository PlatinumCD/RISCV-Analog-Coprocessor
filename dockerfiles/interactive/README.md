# Analog Interactive Stack Development Environment

This repository is designed to help you build and run a Docker-based development environment for the Analog Interactive Stack. The environment is tailored for cross-compilation and includes essential tools and configurations.

## Prerequisites

- Docker
- Make

Ensure that Docker and Make are installed on your system before proceeding.

## Docker Images

This setup uses two main Docker images:

1. `analog-llvm-riscv-musl`
2. `analog-sst-crosssim-dev`

These images are pulled from the Docker Hub user specified in the `Makefile`.

## Build Arguments

The following arguments are used to configure the Docker build:

- `DOCKERHUB_USER`: Docker Hub user where the images are stored. Default is `platinumcd`.
- `RELEASE`: Release version of the images. Default is `1.0.0`.
- `LLVM_RISCV_MUSL_IMAGE_NAME`: Name of the LLVM RISC-V MUSL image. Default is `analog-llvm-riscv-musl`.
- `SST_CROSSSIM_IMAGE_NAME`: Name of the SST CrossSim image. Default is `analog-sst-crosssim-dev`.

These arguments can be overridden in the `Makefile`.

## Build and Run the Docker Image

### To Build the Docker Image

Run the following command:

```sh
make image
```

This command will build the Docker image using the specified arguments.

### To Run the Docker Container

Run the following command:

```sh
make run
```

This command will spin up a Docker container using the newly built image and map the current working directory to `/local` inside the container.

## Dockerfile Explanation

The Dockerfile uses a multi-stage build:

- The first stage pulls the `analog-llvm-riscv-musl` image and sets up the build environment.
- The second stage pulls the `analog-sst-crosssim-dev` image and copies necessary files from the first stage.

It also installs `vim` and sets up environment variables and a working directory.

## Custom Vim Configuration

A custom Vim configuration file located at `./etc/vimrc` is copied to the root user's home directory in the container.

## Environment Variables

The following environment variables are set in the Docker container:

- `PATH`: Includes the LLVM binary directory.
- `BUILD_SRC`: Source directory for builds.
- `BUILD_DEST`: Destination directory for builds.

## Makefile Explanation

The Makefile includes tasks to build and run the Docker image:

- `run`: Runs a Docker container using the built image.
- `image`: Builds the Docker image with the specified arguments.

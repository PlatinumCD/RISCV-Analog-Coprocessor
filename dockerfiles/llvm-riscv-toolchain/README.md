# LLVM-RISC-V Toolchain Dockerfile README

## Introduction

This repository uses a Dockerfile and Makefile to build an image encapsulating the LLVM and RISC-V toolchain environment. The Makefile hosts commands for initiating the build of Docker containers and images, cleaning them, and deploying them.

## Prerequisites

1. Docker - Docker must be installed on your machine to build and run Docker containers and images.
2. Make - The GNU make utility is used to manage the Dockerfile compilation process and to handle the commands utilized within the Makefile.

## Usage

### Dockerfile

The Dockerfile provides instructions for creating a Docker image that consists of the LLVM and the RISC-V toolchain:

- The base image being used is Ubuntu version 22.04.
- The Dockerfile constructs two directories: `/src` and `/opt`.
- Dependencies crucial to building the riscv-gnu-toolchain and LLVM (including git, cmake, python3, and others) are installed.
- The Dockerfile clones the riscv-gnu-toolchain project into the `/src/riscv` directory and then compiles it.
- Likewise, the llvm-project is cloned into the `/src/llvm-project` directory and built.
- Lastly, the LLVM binaries are added to the PATH.

### Makefile commands

1. `all`: This command incites the process of building the LLVM-RISC-V toolchain image. You can initiate this process using `make all`.

2. `clean`: This command will stop a running Docker container, and also remove the Docker image. You can perform this operation using `make clean`.

3. `run`: Starting the Docker container in interactive mode can be done using the command `make run`.

## Modifying Dockerfile Arguments

The Dockerfile makes use of environment variables to further customization. 

1. `ENV RACS_SRC=/src` - This variable helps specify the directory for source files. You can replace `/src` with the directory of your choice. 

2. `ENV RACS_BUILD=/opt` - Determines the directory where the build files will reside. In case you want the files to reside in a different directory, replace `/opt` with your preferred directory. 

3. `ENV NUM_THREADS=64` - Defines the number of CPU cores to participate in building. Prior to initiation, ensure your machine has sufficient resources. You can modify `NUM_THREADS` as per the number cores available on your machine by replacing `64`.

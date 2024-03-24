# LLVM-RISC-V Toolchain Dockerfile 

__The only difference between the `llvm-riscv-toolchain` Dockerfile and the `llvm-riscv-toolchain-dev` Dockerfile is that the source directory for LLVM and RISC-V are removed the `llvm-riscv-toolchain` Dockerfile.__

This makes a huge difference in terms of Docker image size.
```
$ docker image ls
REPOSITORY                 TAG       IMAGE ID       CREATED         SIZE
llvm-riscv-toolchain       latest    72aac8cdfdc8   2 minutes ago   1.93GB
llvm-riscv-toolchain-dev   latest    9e5f0412deb5   2 hours ago     20.2GB
```

## Introduction

This directory uses a Dockerfile and Makefile to build an image encapsulating the LLVM and RISC-V toolchain environment. The Makefile hosts commands for initiating the build of Docker containers and images, cleaning them, and deploying them.

This directory also includes a `c_compiler` and `cpp_compiler` script that compiles programs locally using the Docker container exectuables. The Docker container must be built in order for these scripts to run.

## Prerequisites

1. Docker - Docker must be installed on your machine to build and run Docker containers and images.
2. Make - The GNU make utility is used to manage the Dockerfile compilation process and to handle the commands utilized within the Makefile.

## Usage

### Dockerfile

The Dockerfile provides instructions for creating a Docker image that consists of the LLVM and the RISC-V toolchain:

- The base image being used is Ubuntu version 22.04.
- The Dockerfile constructs two directories: `/src` and `/opt`. These directories can be changed by modifying the Dockerfile arguments.
- Dependencies to building the riscv-gnu-toolchain and LLVM (including git, cmake, python3, and others) are installed.
- The Dockerfile clones the riscv-gnu-toolchain project into the `/src/riscv` directory and then compiles it.
- Likewise, the llvm-project is cloned into the `/src/llvm-project` directory and built.
- Lastly, the LLVM binaries are added to the PATH.

### Makefile commands

1. `all`: This command incites the process of building the LLVM-RISC-V toolchain image. You can initiate this process using `make all`.

2. `clean`: This command will stop a running Docker container, and also remove the Docker image. You can perform this operation using `make clean`.

3. `run`: Starting the Docker container in interactive mode can be done using the command `make run`.

4. `test`: This command will use the `c_compiler` and `cpp_compiler` script to build the cases in the `tests` directory. This will determine if the compiler was successfully built in the Docker container.

5. `test-clean`: This command will remove all `.out` files in the `tests` directory created from `make test`

## Modifying Dockerfile Arguments

The Dockerfile makes use of environment variables to further customization. Any modification to the Dockerfile arguments will require a similar argument change in the `c_compiler` and `cpp_compiler` scripts to work properly.

1. `ENV RACS_SRC=/src` - This variable helps specify the directory for source files. You can replace `/src` with the directory of your choice. 

2. `ENV RACS_BUILD=/opt` - Determines the directory where the build files will reside. In case you want the files to reside in a different directory, replace `/opt` with your preferred directory. 

3. `ENV NUM_THREADS=64` - Defines the number of CPU cores to participate in building. Prior to initiation, ensure your machine has sufficient resources. You can modify `NUM_THREADS` as per the number cores available on your machine by replacing `64`.

## Using `c_compiler` and `cpp_compiler`

__After__ the Dockerfile is built, the scripts `c_compiler` and `cpp_compiler` will work as intended. You can use these scripts like a native compiler. Here is an example of the LLVM RISC-V compiler being used to compile a program from __outside__ the container:

```
./c_compiler tests/hello_world.c
```

This command will create the file `tests/hello_world_c.out`.

The scripts only work when the Dockerfile is built because the program is compiled in the container. Here is the script for the `c_compiler`:

```
#!/bin/bash                                                                                                                     

RACS_BUILD=/opt

CC=clang
SYSROOT=$RACS_BUILD/riscv/riscv64-unknown-elf
GCC_TOOLCHAIN=$RACS_BUILD/riscv
TARGET=riscv64
ARCH=rv64g

docker run               \
    --rm -v              \
    $(pwd):/src          \
    llvm-riscv-toolchain \
    $CC                  \
        --sysroot=$SYSROOT             \
        --gcc-toolchain=$GCC_TOOLCHAIN \
        --target=$TARGET               \
        -march=$ARCH                   \
        /src/$1 -o /src/"${1%.c}_c.out"
```

The current directory is mounted into the container and compiled. 

# Comprehensive Development Environment for Analog, RISC-V, LLVM, and SST CrossSim

This repository contains a multi-faceted development environment that facilitates the integration and interaction between various sophisticated systems: the Analog library, RISC-V toolchain, LLVM, and SST CrossSim. The project is structured into several subdirectories, each encapsulating the Dockerized setup required to streamline the build and deployment processes for these systems.

## Directory Structure

- `analog-stack-dev/`: Builds the Analog Stack integrating LLVM and SST CrossSim.
- `llvm-riscv-musl-dev/`: Provides the setup to compile and build LLVM targeting RISC-V architecture with MUSL.
- `riscv-musl-dev/`: Contains the setup required to build a RISC-V toolchain with MUSL support.
- `sst-crosssim-dev/`: Focuses on setting up SST CrossSim, a framework for simulating computer architecture.

## Building the Environment

### Prerequisites

1. Docker
2. Make

### Steps to Build

1. **Build the Entire Environment**:
   From the top-level directory, you can build all required Docker images using:
   ```sh
   make build
   ```

2. **Run the SST CrossSim Example**:
   After the build process is complete, you can launch the SST CrossSim example using:
   ```sh
   make run
   ```

## Subdirectory Details

### analog-stack-dev

Contains the Docker setup to integrate the Analog library with the LLVM RISC-V compiler and SST CrossSim. This image is the culminating build merging all components. Commands available in this subdirectory:

- Build the image:
  ```sh
  make image
  ```

- Push the image to Docker Hub:
  ```sh
  make push
  ```

### llvm-riscv-musl-dev

Sets up an environment to build the LLVM compiler targeting the RISC-V architecture with MUSL. The provided Makefile and Dockerfile detail the required build steps and dependencies. Commands available in this subdirectory:

- Build the image:
  ```sh
  make image
  ```

- Push the image to Docker Hub:
  ```sh
  make push
  ```

- Run tests:
  ```sh
  make test
  ```

- Clean test outputs:
  ```sh
  make clean-test
  ```

### riscv-musl-dev

Focuses on downloading and building the RISC-V toolchain with MUSL support, including compilers and linkers necessary for cross-compiling applications. Commands available in this subdirectory:

- Build the image:
  ```sh
  make image
  ```

- Push the image to Docker Hub:
  ```sh
  make push
  ```

### sst-crosssim-dev

Prepares the environment to install and set up SST CrossSim, a simulation framework used for modeling and studying computer architectures. This setup includes necessary dependencies and configurations. Commands available in this subdirectory:

- Build the image:
  ```sh
  make image
  ```

- Push the image to Docker Hub:
  ```sh
  make push
  ```

## Main Application

The `src` directory contains example code and scripts demonstrating the use of the analog library in conjunction with SST CrossSim. The top-level `Makefile` orchestrates the build process across all subdirectories, ensuring seamless integration.

## Conclusion

This comprehensive setup allows developers and researchers to work with complex simulations and compiler toolchains using Docker containers. The layered architecture enables easy modification, testing, and integration of different components, facilitating advanced development workflows in computer architecture simulations and compiler construction.
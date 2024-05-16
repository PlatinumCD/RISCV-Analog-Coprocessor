# Infrastructure Dockerfiles

The `prod`, `dev`, and `interactive` directories contain scripts to build the containerized analog stack infrastructure using Docker. The analog stack infrastructure consists of two major components:

1. The Compiler toolchain
2. The Simulation framework

## The Compiler toolchain

The compiler toolchain employs a custom LLVM RISC-V musl cross compiler. This custom compiler ensures compatibility with the simulation infrastructure, facilitating the compilation process tailored to the specific requirements of the analog stack.

## The Simulation infrastructure

The simulation framework leverages SST (Structural Simulation Toolkit) and integrates a custom SST element named Golem. Golem includes CrossSim to implement analog capabilities, enabling detailed modeling and simulation of analog behaviors in conjunction with digital simulations.

# Prod vs Dev vs Interactive

The primary difference between `prod`, `dev`, and `interactive` images lies in the amount of content included:

- **prod**: This image does not contain the source code for the binaries that are built. It is optimized for deployment with a focus on being minimal, which results in much smaller image sizes.
- **dev**: This image contains the source code for all the binaries that are built. It provides a comprehensive development environment with all necessary tools and dependencies for development and debugging.
- **interactive**: This image utilizes the `prod` compiler toolchain but includes the `dev` simulation infrastructure. It is designed for interactive use, offering a balance between the lightweight nature of `prod` and the full capabilities of `dev`.

Prod images are significantly smaller than dev images, reflecting their streamlined content without source code.

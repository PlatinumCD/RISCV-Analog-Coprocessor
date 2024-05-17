# RISC-V Analog Coprocessor

This repository contains the build infrastructure for integrating and utilizing several key components in the RISC-V Analog Coprocessor project. These include the Analog library, a custom LLVM toolchain, custom SST elements, and the build scripts. This setup supports a seamless development, production, and interactive usage experience.

## Components

1. **Analog Library**: [Analog Library](https://github.com/PlatinumCD/analog-library) (main)
   - Used to implement analog capabilities within the simulation and toolchain.
   
2. **Custom LLVM Toolchain**: [LLVM Project](https://github.com/PlatinumCD/llvm-project) (llvm-riscv)
   - A custom LLVM RISC-V musl cross-compiler that ensures compatibility with the analog stack.

3. **Custom SST Elements**: [SST Elements](https://github.com/PlatinumCD/sst-elements/tree/refactor) (refactor)
   - Provides elements from the Structural Simulation Toolkit (SST), including Golem and CrossSim, to enable the simulation of analog behaviors.

4. **Build Infrastructure**: [Build Infrastructure](https://github.com/PlatinumCD/RISCV-Analog-Coprocessor) (main)
   - Contains Dockerfiles and Makefiles required to set up the entire build environment.

## Building and Running The Environment

The environment can be built in three different ways: **Production**, **Development**, and **Interactive**. Each approach uses Docker for containerization but serves different use cases.

### Production Environment

The production environment is optimized for deployment. It does not contain the source code of built binaries, making the images smaller and ideal for execution.

To build and run the production environment:
```sh
cd dockerfiles/prod
make build
make run
```

### Development Environment

The development environment includes the source code and all necessary tools and dependencies for developing and debugging the software.

To build and run the development environment:
```sh
cd dockerfiles/dev
make build
make run
```

### Interactive Environment

The interactive environment strikes a balance between the production and development setups. It uses the production compiler toolchain but includes the development simulation infrastructure, allowing for interactive use.

To build and run the interactive environment:
```sh
cd dockerfiles/interactive
make image
make run
```

## Conclusion

This setup integrates complex simulations and compiler toolchains using Docker containers. By leveraging analog capabilities, a custom LLVM toolchain, and custom SST elements, developers and researchers can perform advanced simulations and compiler construction in a streamlined environment.

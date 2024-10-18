# RISC-V Analog Coprocessor

The RISC-V Analog Coprocessor project is designed to extend the RISC-V architecture with analog processing capabilities, enabling the simulation and development of systems that integrate both digital and analog components. This environment provides a custom LLVM-based toolchain and Structural Simulation Toolkit (SST) elements, including Golem and CrossSim, to facilitate seamless simulation of analog behavior within RISC-V systems.

By integrating an SST simulator infrastructure specifically tailored for analog applications, this project allows researchers and developers to explore the interaction between analog and digital processing in a unified simulation environment. The analog stack is fully supported by a cross-compiling toolchain and custom SST elements, making it possible to simulate, prototype, and analyze analog components alongside standard digital workflows.

## Components

1. **Custom LLVM Toolchain**: [LLVM Project](https://github.com/PlatinumCD/llvm-project) (llvm-riscv)
   - A modified version of LLVM with custom instructions designed for the analog coprocessor.

2. **Build Infrastructure**: [Build Infrastructure](https://github.com/PlatinumCD/RISCV-Analog-Coprocessor) (main)
   - Contains Dockerfiles and Makefiles required to set up the entire build environment.

3. **Custom SST Elements**: [SST Elements](https://github.com/PlatinumCD/sst-elements/tree/refactor) (refactor)
   - Provides elements from the Structural Simulation Toolkit (SST), including Golem and CrossSim, to enable the simulation of analog programs.

4. **Analog Library**: [Analog Library](https://github.com/PlatinumCD/analog-library) (main)
   - A C++ library designed to assist in the creation of analog applications.

## How to use

Build the RISCV musl Toolchain:
```bash
cd dockerfiles/riscv-musl
make
```

Build the LLVM RISCV cross-compiler (Relies on RISCV musl Toolchain):
```bash
cd dockerfiles/llvm-riscv-musl
make
```

Build the SST Core, SST Elements, and CrossSim Image (Incomplete):
```bash
cd dockerfiles/sst-crosssim
make
```

## Conclusion

This setup integrates complex simulations and compiler toolchains using Docker containers. By leveraging analog capabilities, a custom LLVM toolchain, and custom SST elements, developers and researchers can perform advanced simulations and compiler construction in a streamlined environment.

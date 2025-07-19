# analog_riscv_toolchain.cmake

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# Access BUILD_DEST from the environment
set(BUILD_DEST $ENV{BUILD_DEST})

# Ensure BUILD_DEST is defined
if(NOT BUILD_DEST)
  message(FATAL_ERROR "BUILD_DEST is not set. Please set the BUILD_DEST environment variable.")
endif()

set(CMAKE_SYSROOT ${BUILD_DEST}/riscv/sysroot)

set(CMAKE_C_COMPILER ${BUILD_DEST}/riscv/bin/riscv64-unknown-linux-musl-gcc)
set(CMAKE_CXX_COMPILER ${BUILD_DEST}/riscv/bin/riscv64-unknown-linux-musl-g++)
set(CMAKE_AR ${BUILD_DEST}/riscv/bin/riscv64-unknown-linux-musl-ar)
set(CMAKE_RANLIB ${BUILD_DEST}/riscv/bin/riscv64-unknown-linux-musl-ranlib)
set(CMAKE_LINKER ${BUILD_DEST}/riscv/bin/riscv64-unknown-linux-musl-ld)

set(CMAKE_FIND_ROOT_PATH ${BUILD_DEST}/riscv/sysroot)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

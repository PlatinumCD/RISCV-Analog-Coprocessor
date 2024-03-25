docker buildx build                                  \
    --push                                           \
    --platform linux/arm64,linux/amd64               \
    --tag platinumcd/llvm-riscv-toolchain:dev-latest \
    .

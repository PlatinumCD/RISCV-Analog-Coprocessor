MAKE_TARGET?=image

pushd dev

pushd sst-crosssim-dev
make $MAKE_TARGET
popd

pushd riscv-musl-dev
make $MAKE_TARGET
popd

pushd llvm-riscv-musl-dev
make $MAKE_TARGET
popd

pushd analog-stack-dev
make $MAKE_TARGET
popd

popd

MAKE_TARGET?=image

pushd prod 

pushd sst-crosssim
make $MAKE_TARGET
popd

pushd riscv-musl
make $MAKE_TARGET
popd

pushd llvm-riscv-musl
make $MAKE_TARGET
popd

pushd analog-stack
make $MAKE_TARGET
popd

popd

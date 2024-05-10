# Prod vs Dev

The difference between `prod` and `dev` is that `prod` doesn't include the
source code used to build the infrastructure. Also, `prod` takes up
significantly less space.

REPOSITORY                              TAG       IMAGE ID       CREATED          SIZE
platinumcd/analog-stack-dev             1.0.0     7ce6745ca002   2 minutes ago    24.6GB
platinumcd/analog-llvm-riscv-musl-dev   1.0.0     05ec5af55b42   4 minutes ago    16.7GB
platinumcd/analog-riscv-musl-dev        1.0.0     55971c56ea1d   31 minutes ago   12GB
platinumcd/analog-sst-crosssim-dev      1.0.0     747330d96143   5 hours ago      8.47GB

REPOSITORY                              TAG       IMAGE ID       CREATED          SIZE
platinumcd/analog-stack                 1.0.0     8bbdb32c8517   2 hours ago      3.87GB
platinumcd/analog-llvm-riscv-musl       1.0.0     cc3717f5832b   5 hours ago      2.21GB
platinumcd/analog-riscv-musl            1.0.0     fe168b1a2ed3   6 hours ago      1.88GB
platinumcd/analog-sst-crosssim          1.0.0     2693d5e9b53b   2 hours ago      2.28GB

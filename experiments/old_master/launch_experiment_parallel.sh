#!/usr/bin/env bash
set -eo pipefail

# ================= Compiler ================= #
export ROOT="$(realpath "$(pwd)"/..)"
export BUILD_DEST="$ROOT/build"
COMPILER="$BUILD_DEST/llvm-project/bin/clang++"
TARGET="riscv64-unknown-linux-musl"
TOOLCHAIN="$BUILD_DEST/riscv-gnu-toolchain"
SYSROOT="$TOOLCHAIN/sysroot"

RCC="riscv64-unknown-linux-musl-g++"
RCXX_FLAGS="-static -fopenmp"

CC="$COMPILER --target=$TARGET --gcc-toolchain=$TOOLCHAIN --sysroot=$SYSROOT"
CXX_FLAGS="-static" 


# ================= SST Setup ================ #
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DEST/python3.11/lib:$BUILD_DEST/isl/lib"
export PATH="$PATH:$BUILD_DEST/python3.11/bin:$BUILD_DEST/llvm-project/bin:$BUILD_DEST/sst-core/bin"
export PATH="$PATH:$TOOLCHAIN/bin"
module load openmpi


# ================= Params =================== #
export GOLEM_ARRAY_TYPE="golem.CrossSimFloatArray"
NUM_ARRAYS_LIST=(64 32 16 8 4 2 1)
NUM_VCORES_LIST=(1 2 4 8 16 32 64)

# Build list of .cpp files
SRC_DIR=$(pwd)/src_biconj
mapfile -t CPP_FILES < <(ls -1 $SRC_DIR/*.cpp)

N_CPP=${#CPP_FILES[@]}
N_PAIRS=${#NUM_ARRAYS_LIST[@]}
TOTAL=$(( N_CPP * N_PAIRS ))

# For local testing without Slurm
: "${SLURM_ARRAY_TASK_ID:=0}"


if (( SLURM_ARRAY_TASK_ID < 0 || SLURM_ARRAY_TASK_ID >= TOTAL )); then
  echo "Error: SLURM_ARRAY_TASK_ID=${SLURM_ARRAY_TASK_ID} out of range [0, $((TOTAL-1))]"
  exit 1
fi

# ======== Map array index -> (cpp, num_arrays, input) ======== #
idx=$SLURM_ARRAY_TASK_ID

i_cpp=$(( idx / N_PAIRS ))
i_pair=$(( idx % N_PAIRS ))

CPP_FILE="${CPP_FILES[$i_cpp]}"
ALGORITHM_NAME="$(basename "$CPP_FILE" .cpp)"
export GOLEM_NUM_ARRAYS="${NUM_ARRAYS_LIST[$i_pair]}"
export VANADIS_NUM_CORES="${NUM_VCORES_LIST[$i_pair]}"
export ARRAY_INPUT_SIZE=64
export ARRAY_OUTPUT_SIZE=64


TRIAL_NAME="${ALGORITHM_NAME}-${GOLEM_NUM_ARRAYS}-${VANADIS_NUM_CORES}-${ARRAY_INPUT_SIZE}"
RESULTS_DIR="results/$TRIAL_NAME"
mkdir -p $RESULTS_DIR

TARGET_EXE="$RESULTS_DIR/${TRIAL_NAME}.exe"


echo "Trial: $TRIAL_NAME"
echo "  Algorithm: $ALGORITHM_NAME"
echo "  Num arrays: $GOLEM_NUM_ARRAYS"
echo "  Size: ${ARRAY_INPUT_SIZE}x${ARRAY_OUTPUT_SIZE}"
echo "  Source: $CPP_FILE"
echo "  Target: $TARGET_EXE"



# ============== Test Zone ================ #
#$CC $CXX_FLAGS -c kernel.cpp -o kernel.o

#$RCC $RCXX_FLAGS \
#  -DINPUT_SIZE="$INPUT_SIZE" \
#  -DNUM_ARRAYS="$GOLEM_NUM_ARRAYS" \
#  test.cpp kernel.o -o test.exe

#export VANADIS_NUM_CORES=20
#export GOLEM_NUM_ARRAYS=1
#export VANADIS_EXE=$(pwd)/test.exe


#export ARRAY_INPUT_SIZE=2
#export ARRAY_OUTPUT_SIZE=2

#sst sst-runtime-configuration.py
#srun --nodelist=kn15,kn16 -N 2 -n 20 sst sst-runtime-configuration.py
#exit

# ================= Build & Run ================= #

$CC $CXX_FLAGS -c kernel.cpp -o kernel.o

$RCC $RCXX_FLAGS \
  -DNUM_ARRAYS="$GOLEM_NUM_ARRAYS" \
  -DNUM_CORES="$VANADIS_NUM_CORES" \
  "$CPP_FILE" kernel.o -o "$TARGET_EXE"

echo "  Build: OK"

export VANADIS_EXE="$ROOT/experiment/$TARGET_EXE"
cp sst-runtime-configuration.py $RESULTS_DIR/.
pushd $RESULTS_DIR
    sst sst-runtime-configuration.py > "sst_stats.data"
popd
echo "  Done → $RESULTS_DIR"


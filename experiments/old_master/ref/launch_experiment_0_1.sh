#!/usr/bin/env bash
set -eo pipefail

# ================= Compiler ================= #
export ROOT="$(realpath "$(pwd)"/..)"
export BUILD_DEST="$ROOT/build"
COMPILER="$BUILD_DEST/llvm-project/bin/clang++"
TARGET="riscv64-unknown-linux-musl"
TOOLCHAIN="$BUILD_DEST/riscv-gnu-toolchain"
SYSROOT="$TOOLCHAIN/sysroot"
CC="$COMPILER --target=$TARGET --gcc-toolchain=$TOOLCHAIN --sysroot=$SYSROOT"
CXX_FLAGS="-static"

# ================= SST Setup ================ #
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DEST/python3.11/lib"
export PATH="$PATH:$BUILD_DEST/python3.11/bin:$BUILD_DEST/llvm-project/bin:$BUILD_DEST/sst-core/bin"
module load openmpi

# ================= Params =================== #
export GOLEM_ARRAY_TYPE="golem.CrossSimFloatArray"
NUM_ARRAYS_LIST=(0 1)
INPUT_SIZE_LIST=(4 8 16 32 64)

# Build list of .cpp files
mapfile -t CPP_FILES < <(ls -1 src/*.cpp)

N_CPP=${#CPP_FILES[@]}
N_NUM=${#NUM_ARRAYS_LIST[@]}
N_INP=${#INPUT_SIZE_LIST[@]}
TOTAL=$(( N_CPP * N_NUM * N_INP ))

# For local testing without Slurm
: "${SLURM_ARRAY_TASK_ID:=0}"

if (( SLURM_ARRAY_TASK_ID < 0 || SLURM_ARRAY_TASK_ID >= TOTAL )); then
  echo "Error: SLURM_ARRAY_TASK_ID=${SLURM_ARRAY_TASK_ID} out of range [0, $((TOTAL-1))]"
  exit 1
fi

# ======== Map array index -> (cpp, num_arrays, input) ======== #
idx=$SLURM_ARRAY_TASK_ID
block=$(( N_NUM * N_INP ))

i_cpp=$(( idx / block ))
rem=$(( idx % block ))
i_num=$(( rem / N_INP ))
i_inp=$(( rem % N_INP ))

CPP_FILE="${CPP_FILES[$i_cpp]}"
ALGORITHM_NAME="$(basename "$CPP_FILE" .cpp)"
GOLEM_NUM_ARRAYS="${NUM_ARRAYS_LIST[$i_num]}"
INPUT_SIZE="${INPUT_SIZE_LIST[$i_inp]}"

export GOLEM_NUM_ARRAYS
export ARRAY_INPUT_SIZE="$INPUT_SIZE"
export ARRAY_OUTPUT_SIZE="$INPUT_SIZE"

TRIAL_NAME="${ALGORITHM_NAME}-${ARRAY_INPUT_SIZE}-${ARRAY_OUTPUT_SIZE}-${GOLEM_NUM_ARRAYS}"

TARGET_EXE="src/${TRIAL_NAME}.exe"

echo "Trial: $TRIAL_NAME"
echo "  Algorithm: $ALGORITHM_NAME"
echo "  Num arrays: $GOLEM_NUM_ARRAYS"
echo "  Size: ${ARRAY_INPUT_SIZE}x${ARRAY_OUTPUT_SIZE}"
echo "  Source: $CPP_FILE"
echo "  Target: $TARGET_EXE"

# ================= Build & Run ================= #
$CC $CXX_FLAGS \
  -DINPUT_SIZE="$INPUT_SIZE" \
  -DNUM_ARRAYS="$GOLEM_NUM_ARRAYS" \
  "$CPP_FILE" -o "$TARGET_EXE"

echo "  Build: OK"

export VANADIS_EXE="$ROOT/experiment/$TARGET_EXE"
mkdir -p "results/$TRIAL_NAME"
cp sst-runtime-configuration.py results/$TRIAL_NAME/.
pushd results/$TRIAL_NAME
    sst sst-runtime-configuration.py > "sst_stats.data"
popd
echo "  Done → results/$TRIAL_NAME"


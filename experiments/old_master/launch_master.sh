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

# Pairs must be same length
NUM_ARRAYS_LIST=(64 32 16 8 4 2 1)
NUM_VCORES_LIST=(1  2  4  8 16 32 64)

SRC_DIR=${SRC_DIR:-"$(pwd)/src_master"}
CONFIG_DIR=${CONFIG_DIR:-"$(pwd)/configs"}

shopt -s nullglob
mapfile -t CPP_FILES    < <(printf '%s\n' "$SRC_DIR"/*.cpp)
mapfile -t CONFIG_FILES < <(printf '%s\n' "$CONFIG_DIR"/*.py)
shopt -u nullglob

N_CPP=${#CPP_FILES[@]}
N_CONF=${#CONFIG_FILES[@]}
N_PAIRS=${#NUM_ARRAYS_LIST[@]}

if (( N_CPP==0 || N_CONF==0 )); then
  echo "No CPP or CONFIG files found. SRC_DIR='$SRC_DIR' CONFIG_DIR='$CONFIG_DIR'"; exit 1
fi
if (( N_PAIRS != ${#NUM_VCORES_LIST[@]} )); then
  echo "NUM_ARRAYS_LIST and NUM_VCORES_LIST length mismatch"; exit 1
fi

TOTAL=$(( N_CPP * N_PAIRS * N_CONF ))
: "${SLURM_ARRAY_TASK_ID:=0}"
idx=$SLURM_ARRAY_TASK_ID
if (( idx < 0 || idx >= TOTAL )); then
  echo "Error: SLURM_ARRAY_TASK_ID=${idx} out of range [0, $((TOTAL-1))]"; exit 1
fi

# ==== index -> (cpp, pair, conf) ====
block=$(( N_PAIRS * N_CONF ))
i_cpp=$(( idx / block ))
rem=$(( idx % block ))
i_pair=$(( rem / N_CONF ))
i_conf=$(( rem % N_CONF ))

CPP_FILE="${CPP_FILES[$i_cpp]}"
ALGORITHM_NAME="$(basename "$CPP_FILE" .cpp)"

GOLEM_NUM_ARRAYS="${NUM_ARRAYS_LIST[$i_pair]}"
VANADIS_NUM_CORES="${NUM_VCORES_LIST[$i_pair]}"

CONFIG_FILE="${CONFIG_FILES[$i_conf]}"
CONFIG_NAME="$(basename "$CONFIG_FILE" .py)"

TRIAL_NAME="${ALGORITHM_NAME}-${GOLEM_NUM_ARRAYS}-${VANADIS_NUM_CORES}-${CONFIG_NAME}"

export GOLEM_NUM_ARRAYS="${NUM_ARRAYS_LIST[$i_pair]}"
export VANADIS_NUM_CORES="${NUM_VCORES_LIST[$i_pair]}"
export ARRAY_INPUT_SIZE=128
export ARRAY_OUTPUT_SIZE=128

RESULTS_DIR="results/$TRIAL_NAME"
mkdir -p $RESULTS_DIR

TARGET_EXE="$RESULTS_DIR/${TRIAL_NAME}.exe"


echo "Trial: $TRIAL_NAME"
echo "  Algorithm: $ALGORITHM_NAME"
echo "  Num arrays: $GOLEM_NUM_ARRAYS"
echo "  Size: ${ARRAY_INPUT_SIZE}x${ARRAY_OUTPUT_SIZE}"
echo "  Source: $CPP_FILE"
echo "  Target: $TARGET_EXE"
echo "  Config: $CONFIG_NAME"


# ================= Build & Run ================= #

$CC $CXX_FLAGS -c kernel.cpp -o kernel.o

$RCC $RCXX_FLAGS \
  -DNUM_ARRAYS="$GOLEM_NUM_ARRAYS" \
  -DNUM_CORES="$VANADIS_NUM_CORES" \
  "$CPP_FILE" kernel.o -o "$TARGET_EXE"

echo "  Build: OK"

export VANADIS_EXE="$ROOT/experiment/$TARGET_EXE"
cp $CONFIG_FILE $RESULTS_DIR/.
pushd $RESULTS_DIR
    sst "$CONFIG_NAME.py" > "sst_stats.data"
popd
echo "  Done → $RESULTS_DIR"


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
NUM_VCORES_LIST=(16 32 64)

SRC_DIR=${SRC_DIR:-"$(pwd)/src_baseline"}
CONFIG_DIR=${CONFIG_DIR:-"$(pwd)/configs_baseline"}

shopt -s nullglob
mapfile -t CPP_FILES    < <(printf '%s\n' "$SRC_DIR"/*.cpp)
mapfile -t CONFIG_FILES < <(printf '%s\n' "$CONFIG_DIR"/*.py)
shopt -u nullglob

N_CPP=${#CPP_FILES[@]}
N_CONF=${#CONFIG_FILES[@]}
N_PAIRS=${#NUM_VCORES_LIST[@]}

if (( N_CPP==0 || N_CONF==0 )); then
  echo "No CPP or CONFIG files found. SRC_DIR='$SRC_DIR' CONFIG_DIR='$CONFIG_DIR'"; exit 1
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

export VANADIS_NUM_CORES="${NUM_VCORES_LIST[$i_pair]}"

CONFIG_FILE="${CONFIG_FILES[$i_conf]}"
CONFIG_NAME="$(basename "$CONFIG_FILE" .py)"

TRIAL_NAME="baseline-${ALGORITHM_NAME}-${VANADIS_NUM_CORES}-${CONFIG_NAME}"

RESULTS_DIR="results/$TRIAL_NAME"
mkdir -p $RESULTS_DIR

TARGET_EXE="$RESULTS_DIR/${TRIAL_NAME}.exe"


echo "Trial: $TRIAL_NAME"
echo "  Algorithm: $ALGORITHM_NAME"
echo "  Num arrays: $VANADIS_NUM_CORES"
echo "  Source: $CPP_FILE"
echo "  Target: $TARGET_EXE"
echo "  Config: $CONFIG_NAME"


# ================= Build & Run ================= #

$CC $CXX_FLAGS -c kernel.cpp -o kernel.o

$RCC $RCXX_FLAGS \
  -DNUM_CORES="$VANADIS_NUM_CORES" \
  "$CPP_FILE" kernel.o -o "$TARGET_EXE"

echo "  Build: OK"

export VANADIS_EXE="$ROOT/experiment/$TARGET_EXE"
cp $CONFIG_FILE $RESULTS_DIR/.
pushd $RESULTS_DIR
    sst "$CONFIG_NAME.py" > "sst_stats.data"
popd
echo "  Done → $RESULTS_DIR"


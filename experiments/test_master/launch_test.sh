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
. $BUILD_DEST/analog_venv/bin/activate
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DEST/python3.13/lib:$BUILD_DEST/isl/lib:$BUILD_DEST/ncurses/lib"
export PYTHONPATH=$BUILD_DEST/analog_venv/lib/python3.13t/site-packages
export PATH="$PATH:$BUILD_DEST/llvm-project/bin:$BUILD_DEST/sst-core/bin"
export PATH="$PATH:$TOOLCHAIN/bin"
module load openmpi

export PYTHON_GIL=0

# ================= Params =================== #
#export GOLEM_ARRAY_TYPE="golem.CrossSimFloatArray"
export GOLEM_ARRAY_TYPE="golem.MVMFloatArray"

export VANADIS_NUM_CORES=$1
export GOLEM_NUM_ARRAYS=1
export VANADIS_EXE=$(pwd)/test.exe

export ARRAY_INPUT_SIZE=2
export ARRAY_OUTPUT_SIZE=2

echo "Trial: Test"
echo "  Num arrays: $GOLEM_NUM_ARRAYS"
echo "  Num cores: $VANADIS_NUM_CORES"
echo "  Size: ${ARRAY_INPUT_SIZE}x${ARRAY_OUTPUT_SIZE}"


#cores=(1 2 4 8 16 32 64)
#ranks=(1 2 4 8 16)
#ranks=(1)
#threads=(1 2 4 8 16 32)
#
#measure() {
#  local X="$1" Y="$2" st end ns s d4 d4s
#  st=$(date +%s%N)
##  mpirun -n "$X" sst -n "$Y" sst-conf.py >/dev/null 2>&1
#  sst -n "$Y" sst-conf.py >/dev/null 2>&1
#  end=$(date +%s%N)
#  ns=$((end - st))                       # ns
#  s=$((ns/1000000000))                   # seconds
#  d4=$(((ns%1000000000)/100000))         # 4 decimals
#  d4s="$d4"
#  if [ "$d4" -lt 10 ];   then d4s="000$d4"
#  elif [ "$d4" -lt 100 ]; then d4s="00$d4"
#  elif [ "$d4" -lt 1000 ]; then d4s="0$d4"
#  fi
#  echo "$s.$d4s"
#}
#
#out="matrix.csv"
#header="num_cores,mpiranks,numthreads,runtime"
#echo "$header" | tee "$out"
#
#for num_cores in "${cores[@]}"; do
#  for mpiranks in "${ranks[@]}"; do
#    for numthreads in "${threads[@]}"; do
#      export VANADIS_NUM_CORES=$num_cores
#	$CC $CXX_FLAGS -c kernel.cpp -o kernel.o
#	$RCC $RCXX_FLAGS \
#	  -DINPUT_SIZE="$INPUT_SIZE" \
#	  -DNUM_ARRAYS="$GOLEM_NUM_ARRAYS" \
#	  -DNUM_CORES="$VANADIS_NUM_CORES" \
#	  test.cpp kernel.o -o test.exe
#
#      runtime=$(measure "$mpiranks" "$numthreads")
#      line="$num_cores,$mpiranks,$numthreads,$runtime"
#      echo "$line" | tee -a "$out"
#    done
#  done
#done
#


$CC $CXX_FLAGS -c kernel.cpp -o kernel.o
$RCC $RCXX_FLAGS \
  -DINPUT_SIZE="$INPUT_SIZE" \
  -DNUM_ARRAYS="$GOLEM_NUM_ARRAYS" \
  -DNUM_CORES="$VANADIS_NUM_CORES" \
  test.cpp kernel.o -o test.exe

$BUILD_DEST/llvm-project/bin/llvm-objdump -d test.exe > test.s

st=$(date +%s%N)
#$BUILD_DEST/gdb/bin/gdb --args $BUILD_DEST/sst-core/libexec/sstsim.x -n 1 sst-conf.py
sst -n 28 sst-config.py 
end=$(date +%s%N)
ns=$((end - st))                       # ns
s=$((ns/1000000000))                   # seconds
d4=$(((ns%1000000000)/100000))         # 4 decimals
d4s="$d4"
if [ "$d4" -lt 10 ];   then d4s="000$d4"
elif [ "$d4" -lt 100 ]; then d4s="00$d4"
elif [ "$d4" -lt 1000 ]; then d4s="0$d4"
fi
echo "$s.$d4s"


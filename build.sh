set -e

export ROOT=$(pwd)
export UTILS_DIR=$ROOT/utils
export BUILD_SRC=$ROOT/src
export BUILD_DEST=$ROOT/build
export NUM_THREADS=32

export RISCV_GIT=https://github.com/riscv-collab/riscv-gnu-toolchain.git
export RISCV_BRANCH=2025.01.16

export LLVM_GIT=https://github.com/PlatinumCD/llvm-project.git
export LLVM_BRANCH=llvm-riscv

export TORCH_MLIR_GIT=https://github.com/PlatinumCD/torch-mlir.git
export TORCH_MLIR_BRANCH=analog-dialect

export CROSSSIM_GIT=https://github.com/sandialabs/cross-sim.git
export CROSSSIM_BRANCH=v3.0

export SST_CORE_GIT=https://github.com/sstsimulator/sst-core.git
export SST_CORE_BRANCH=v15.0.0_Final

export SST_ELEMENTS_GIT=https://github.com/sstsimulator/sst-elements.git
export SST_ELEMENTS_BRANCH=v15.0.0_Final

mkdir -p $BUILD_SRC
mkdir -p $BUILD_DEST

# ================== Dependencies ====================== # 

# TexInfo
TEXINFO_URL=https://ftp.gnu.org/gnu/texinfo/texinfo-7.2.tar.xz
if [ ! -d $BUILD_SRC/texinfo ] || [ ! -d $BUILD_DEST/texinfo ]; then
    mkdir -p $BUILD_SRC/texinfo
    pushd $BUILD_SRC/texinfo
        wget $TEXINFO_URL
	    tar -xf texinfo-7.2.tar.xz
	    pushd texinfo-7.2
            ./configure --prefix=$BUILD_DEST/texinfo
	        make -j $NUM_THREADS install
	    popd
    popd
fi
export PATH=$PATH:$BUILD_DEST/texinfo/bin

# GMP
GMP_URL=https://gmplib.org/download/gmp/gmp-6.3.0.tar.xz
if [ ! -d $BUILD_SRC/gmp ] || [ ! -d $BUILD_DEST/gmp ]; then
    mkdir -p $BUILD_SRC/gmp
    pushd $BUILD_SRC/gmp
    	wget $GMP_URL
        tar -xf gmp-6.3.0.tar.xz
	    pushd gmp-6.3.0
            ./configure --prefix=$BUILD_DEST/gmp
	        make -j $NUM_THREADS install
	    popd
    popd
fi
export CFLAGS="$CFLAGS -I$BUILD_DEST/gmp/include"
export CXXFLAGS="$CXXFLAGS -I$BUILD_DEST/gmp/include"
export LDFLAGS="$LDFLAGS -L$BUILD_DEST/gmp/lib"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DEST/gmp/lib"


# MPFR
MPFR_URL=https://www.mpfr.org/mpfr-4.2.1/mpfr-4.2.1.tar.xz
if [ ! -d $BUILD_SRC/mpfr ] || [ ! -d $BUILD_DEST/mpfr ]; then
    mkdir -p $BUILD_SRC/mpfr
    pushd $BUILD_SRC/mpfr
        wget $MPFR_URL
        tar -xf mpfr-4.2.1.tar.xz
        pushd mpfr-4.2.1
            ./configure --prefix=$BUILD_DEST/mpfr --with-gmp=$BUILD_DEST/gmp
	        make -j $NUM_THREADS install
	    popd
    popd
fi
export CFLAGS="$CFLAGS -I$BUILD_DEST/mpfr/include"
export CXXFLAGS="$CXXFLAGS -I$BUILD_DEST/mpfr/include"
export LDFLAGS="$LDFLAGS -L$BUILD_DEST/mpfr/lib"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DEST/mpfr/lib"


# MPC
MPC_URL=https://ftp.gnu.org/gnu/mpc/mpc-1.3.1.tar.gz
if [ ! -d $BUILD_SRC/mpc ] || [ ! -d $BUILD_DEST/mpc ]; then
    mkdir -p $BUILD_SRC/mpc
    pushd $BUILD_SRC/mpc
    	wget $MPC_URL
        tar -xf mpc-1.3.1.tar.gz
	    pushd mpc-1.3.1
            ./configure --prefix=$BUILD_DEST/mpc --with-gmp=$BUILD_DEST/gmp --with-mpfr=$BUILD_DEST/mpfr
	        make -j $NUM_THREADS install
	    popd
    popd
fi
export CFLAGS="$CFLAGS -I$BUILD_DEST/mpc/include"
export CXXFLAGS="$CXXFLAGS -I$BUILD_DEST/mpc/include"
export LDFLAGS="$LDFLAGS -L$BUILD_DEST/mpc/lib"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DEST/mpc/lib"


# EXPAT
EXPAT_URL=https://github.com/libexpat/libexpat/releases/download/R_2_6_2/expat-2.6.2.tar.xz
if [ ! -d $BUILD_SRC/expat ] || [ ! -d $BUILD_DEST/expat ]; then
    mkdir -p $BUILD_SRC/expat
    pushd $BUILD_SRC/expat
    	wget $EXPAT_URL
        tar -xf expat-2.6.2.tar.xz
	    pushd expat-2.6.2
            ./configure --prefix=$BUILD_DEST/expat
	        make -j $NUM_THREADS install
	    popd
    popd
fi
export CFLAGS="$CFLAGS -I$BUILD_DEST/expat/include"
export CXXFLAGS="$CXXFLAGS -I$BUILD_DEST/expat/include"
export LDFLAGS="$LDFLAGS -L$BUILD_DEST/expat/lib"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DEST/expat/lib"
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$BUILD_DEST/expat/lib/pkgconfig"


# ISL
ISL_URL=https://libisl.sourceforge.io/isl-0.26.tar.xz
if [ ! -d $BUILD_SRC/isl ] || [ ! -d $BUILD_DEST/isl ]; then
    mkdir -p $BUILD_SRC/isl
    pushd $BUILD_SRC/isl
    	wget $ISL_URL
        tar -xf isl-0.26.tar.xz
	    pushd isl-0.26
            ./configure --prefix=$BUILD_DEST/isl
	        make -j $NUM_THREADS install
	    popd
    popd
fi
export CFLAGS="$CFLAGS -I$BUILD_DEST/isl/include"
export CXXFLAGS="$CXXFLAGS -I$BUILD_DEST/isl/include"
export LDFLAGS="$LDFLAGS -L$BUILD_DEST/isl/lib"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DEST/isl/lib"


# LIBFFI
LIBFFI_URL=https://github.com/libffi/libffi/releases/download/v3.5.0/libffi-3.5.0.tar.gz
if [ ! -d $BUILD_SRC/libffi ] || [ ! -d $BUILD_DEST/libffi ]; then
    mkdir -p $BUILD_SRC/libffi
    pushd $BUILD_SRC/libffi
        wget $LIBFFI_URL
	    tar -xf libffi-3.5.0.tar.gz
	    pushd libffi-3.5.0
	        ./configure --prefix=$BUILD_DEST/libffi
	        make -j $NUM_THREADS install
	    popd
    popd
fi
export CFLAGS="$CFLAGS -I$BUILD_DEST/libffi/include"
export CXXFLAGS="$CXXFLAGS -I$BUILD_DEST/libffi/include"
export LDFLAGS="$LDFLAGS -L$BUILD_DEST/libffi/lib"
export LDFLAGS="$LDFLAGS -L$BUILD_DEST/libffi/lib64"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DEST/libffi/lib"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DEST/libffi/lib64"
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$BUILD_DEST/libffi/lib/pkgconfig"


# LIBTOOL
LIBTOOL_URL=https://ftpmirror.gnu.org/libtool/libtool-2.5.4.tar.gz
if [ ! -d $BUILD_SRC/libtool ] || [ ! -d $BUILD_DEST/libtool ]; then
    mkdir -p $BUILD_SRC/libtool
    pushd $BUILD_SRC/libtool
        wget $LIBTOOL_URL
	    tar -xf libtool-2.5.4.tar.gz
	    pushd libtool-2.5.4
	        ./configure --prefix=$BUILD_DEST/libtool
	        make -j $NUM_THREADS install
	    popd
    popd
fi
export LIBTOOLIZE=$BUILD_DEST/libtool/bin/libtoolize
export LIBTOOL=$BUILD_DEST/libtool/bin/libtool


# PCIACCESS
PCIACCESS_URL=https://www.x.org/releases//individual/lib/libpciaccess-0.16.tar.bz2
if [ ! -d $BUILD_SRC/pciaccess ] || [ ! -d $BUILD_DEST/pciaccess ]; then
    mkdir -p $BUILD_SRC/pciaccess
    pushd $BUILD_SRC/pciaccess
        wget $PCIACCESS_URL
        tar -xf libpciaccess-0.16.tar.bz2
        pushd libpciaccess-0.16
            ./configure --prefix=$BUILD_DEST/pciaccess
            make -j $NUM_THREADS install
        popd
    popd
fi
export CFLAGS="$CFLAGS -I$BUILD_DEST/pciaccess/include"
export CXXFLAGS="$CXXFLAGS -I$BUILD_DEST/pciaccess/include"
export LDFLAGS="$LDFLAGS -L$BUILD_DEST/pciaccess/lib"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DEST/pciaccess/lib"
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$BUILD_DEST/pciaccess/lib/pkgconfig"


# Python3.13
PYTHON_URL=https://www.python.org/ftp/python/3.13.0/Python-3.13.0.tar.xz
if [ ! -d $BUILD_SRC/python3.13 ] || [ ! -d $BUILD_DEST/python3.13 ]; then
    mkdir -p $BUILD_SRC/python3.13
    pushd $BUILD_SRC/python3.13
        wget $PYTHON_URL
        tar -xf Python-3.13.0.tar.xz
        pushd Python-3.13.0
            ./configure --enable-shared \
              --prefix="$BUILD_DEST/python3.13" \
              --with-system-expat --with-system-ffi --disable-gil \
              CPPFLAGS="$CPPFLAGS" LDFLAGS="$LDFLAGS -Wl,--export-dynamic -Wl,-rpath,$BUILD_DEST/python3.13/lib -Wl,-rpath,$BUILD_DEST/python3.13/lib64 -Wl,-rpath,$BUILD_DEST/libffi/lib -Wl,-rpath,$BUILD_DEST/libffi/lib64"
            make -j $NUM_THREADS install
        popd
    popd
fi
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BUILD_DEST/python3.13/lib

# ================= Python + CrossSim ================ #

# Python3.13 Venv
if [ ! -d $BUILD_DEST/analog_venv ]; then
    $BUILD_DEST/python3.13/bin/python3.13 -m venv $BUILD_DEST/analog_venv
fi


# Activate Venv
. $BUILD_DEST/analog_venv/bin/activate
which python3.13


# Install CrossSim
if ! python3.13 -c "import simulator" &>/dev/null; then
    git clone -b $CROSSSIM_BRANCH --depth 1 $CROSSSIM_GIT $BUILD_SRC/cross-sim
    pushd $BUILD_SRC/cross-sim
        python3.13 -m pip install --no-cache-dir .
    popd
fi

pip install --no-cache-dir cmake

# Install PyLibs
if ! command -v ninja &>/dev/null; then
    pip install --no-cache-dir \
      --extra-index-url https://download.pytorch.org/whl/cpu \
      torch torchvision torchaudio \
      numpy pybind11 wheel \
      setuptools cmake ninja \
      packaging pyyaml pillow \
      dill multiprocess # onnx==1.15.0
fi


# ================== RISC-V Toolchain ================== #
if [ ! -d $BUILD_SRC/riscv-gnu-toolchain ] || [ ! -d $BUILD_DEST/riscv-gnu-toolchain ]; then
    git clone -b $RISCV_BRANCH --single-branch $RISCV_GIT $BUILD_SRC/riscv-gnu-toolchain
    pushd $BUILD_SRC/riscv-gnu-toolchain
        ./configure --prefix=$BUILD_DEST/riscv-gnu-toolchain
        make -j $NUM_THREADS musl
    popd 
fi


# ================== LLVM/MLIR + Runtimes (static, RISCV) ================== #
if [ ! -d "$BUILD_SRC/llvm-project" ] || [ ! -d "$BUILD_DEST/llvm-project" ]; then
    git clone -b "$LLVM_BRANCH" --single-branch "$LLVM_GIT" "$BUILD_SRC/llvm-project"

    TARGET="riscv64-unknown-linux-musl"
    TOOLCHAIN="$BUILD_DEST/riscv-gnu-toolchain"
    SYSROOT="$TOOLCHAIN/sysroot"

    RISCV_CLANG_C_COMPILER="$BUILD_DEST/llvm-project/bin/clang"
    RISCV_CLANG_CXX_COMPILER="$BUILD_DEST/llvm-project/bin/clang++"
    RISCV_CLANG_C_FLAGS="--target=$TARGET --gcc-toolchain=$TOOLCHAIN --sysroot=$SYSROOT"
    RISCV_CLANG_CXX_FLAGS="--target=$TARGET --gcc-toolchain=$TOOLCHAIN --sysroot=$SYSROOT"

    # LLVM / MLIR
    pushd $BUILD_SRC/llvm-project
        mkdir -p build
        pushd build
            cmake -G Ninja ../llvm \
              -DCMAKE_INSTALL_PREFIX="$BUILD_DEST/llvm-project" \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_C_COMPILER=gcc \
              -DCMAKE_CXX_COMPILER=g++ \
              -DCMAKE_ASM_COMPILER=gcc \
              -DLLVM_TARGETS_TO_BUILD="RISCV" \
              -DLLVM_ENABLE_PROJECTS="clang;mlir" \
              -DLLVM_DEFAULT_TARGET_TRIPLE="$TARGET" \
              -DLLVM_ENABLE_ASSERTIONS=ON \
              -DBUILD_SHARED_LIBS=OFF \
              -DMLIR_ENABLE_BINDINGS_PYTHON=ON \
              -DLLVM_INSTALL_UTILS=ON \
              -DPython3_EXECUTABLE=$(which python3.13)
            cmake --build . --target install . -j $NUM_THREADS
        popd
    popd

    # LLVM OpenMP
    pushd $BUILD_SRC/llvm-project/openmp
        mkdir build
        pushd build
            cmake .. \
              -DCMAKE_INSTALL_PREFIX=$BUILD_DEST/riscv-gnu-toolchain/sysroot \
              -DCMAKE_C_COMPILER=$RISCV_CLANG_C_COMPILER \
              -DCMAKE_CXX_COMPILER=$RISCV_CLANG_CXX_COMPILER \
              -DCMAKE_C_FLAGS="$RISCV_CLANG_C_FLAGS" \
              -DCMAKE_CXX_FLAGS="$RISCV_CLANG_CXX_FLAGS" \
              -DOPENMP_ENABLE_LIBOMPTARGET=Off \
              -DCMAKE_BUILD_TYPE=Release \
              -DENABLE_OMPT_TOOLS=OFF \
              -DLIBOMP_ARCH=riscv64 \
              -DLIBOMP_HAVE_AS_NEEDED_FLAG=On \
              -DLIBOMP_HAVE_VERSION_SCRIPT_FLAG=On \
              -DLIBOMP_HAVE_STATIC_LIBGCC_FLAG=On \
              -DLIBOMP_HAVE_Z_NOEXECSTACK_FLAG=On \
              -DLIBOMP_OMPD_GDB_SUPPORT=Off \
              -DLIBOMP_ENABLE_SHARED=Off
            cmake --build . --target install . -j $NUM_THREADS
        popd
    popd

    # Torch MLIR
    git clone -b "$TORCH_MLIR_BRANCH" --single-branch "$TORCH_MLIR_GIT" "$BUILD_SRC/torch-mlir"
    pushd $BUILD_SRC/torch-mlir
        mkdir -p build
        pushd build
            cmake -G Ninja -S .. \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_INSTALL_PREFIX=$BUILD_DEST/torch-mlir \
              -DLLVM_DIR=$BUILD_DEST/llvm-project/lib/cmake/llvm \
              -DMLIR_DIR=$BUILD_DEST/llvm-project/lib/cmake/mlir \
              -DTORCH_MLIR_ENABLE_STABLEHLO=OFF \
              -DBUILD_SHARED_LIBS=OFF \
              -DPython3_FIND_VIRTUALENV=STANDARD \
              -DMLIR_ENABLE_BINDINGS_PYTHON=ON \
              -DLLVM_ENABLE_ASSERTIONS=ON \
              -DPython3_EXECUTABLE=$(which python3.13)
            cmake --build . --target install -j $NUM_THREADS
            cp python_packages/torch_mlir/torch_mlir/_mlir_libs/_jit_ir_importer* $BUILD_DEST/torch-mlir/python_packages/torch_mlir/torch_mlir/_mlir_libs/.
        popd
    popd
fi


module load openmpi
# ================= SST Core ================ #
if [ ! -d $BUILD_SRC/sst-core ] || [ ! -d $BUILD_DEST/sst-core ]; then
    git clone -b $SST_CORE_BRANCH --single-branch $SST_CORE_GIT $BUILD_SRC/sst-core
    pushd $BUILD_SRC/sst-core
        ./autogen.sh
        mkdir -p build
        pushd build
            ../configure \
                MPICC=$(which mpicc) \
                MPICXX=$(which mpic++) \
                CFLAGS="-g3 -O0 $CFLAGS" CXXFLAGS="-g3 -O0 $CXXFLAGS" \
                --prefix=$BUILD_DEST/sst-core \
                --with-python=$BUILD_DEST/python3.13/bin/python3.13-config
	        make -j $NUM_THREADS install
	    popd
    popd
fi
export PATH=$PATH:$BUILD_DEST/sst-core/bin

# Build with the flags:
#                CFLAGS="-O3 $CFLAGS" CXXFLAGS="-O3 $CXXFLAGS" \
# for speed



PY_CPPFLAGS="$($BUILD_DEST/python3.13/bin/python3.13-config --includes)"
PY_LDFLAGS="$($BUILD_DEST/python3.13/bin/python3.13-config --ldflags)"
NUMPY_FLAGS="$(python3.13 -c "import numpy; print(numpy.get_include())")"
# ================= SST Elements ================ #
if [ ! -d $BUILD_SRC/sst-elements ] || [ ! -d $BUILD_DEST/sst-elements ]; then
    git clone -b $SST_ELEMENTS_BRANCH --single-branch $SST_ELEMENTS_GIT $BUILD_SRC/sst-elements

    ### This line is because of a Golem problem. A pull request is in order.
    cp $UTILS_DIR/sst-elements/crossSimComputeArray.h $BUILD_SRC/sst-elements/src/sst/elements/golem/array/.
    ###

    pushd $BUILD_SRC/sst-elements
        ./autogen.sh
        mkdir -p build
	    pushd build
	        ../configure \
                MPICC=$(which mpicc) \
                MPICXX=$(which mpic++) \
		CFLAGS="-g3 -O0" CXXFLAGS="-g3 -O0" \
                --prefix=$BUILD_DEST/sst-elements \
                --with-numpy="$NUMPY_FLAGS $PY_CPPFLAGS $PY_LDFLAGS"
	        make -j $NUM_THREADS install
	    popd
    popd
fi

# Build with the flags:
#                CFLAGS="-O3" CXXFLAGS="-O3" \
# for speed


# OPTIONAL debugger
#GDB_URL=https://ftp.gnu.org/gnu/gdb/gdb-14.2.tar.xz
#GDB_URL=https://sourceware.org/pub/gdb/releases/gdb-14.2.tar.xz
#if [ ! -d $BUILD_SRC/gdb ] || [ ! -d $BUILD_DEST/gdb ]; then
#   mkdir -p $BUILD_SRC/gdb
#   pushd $BUILD_SRC/gdb
#    	wget $GDB_URL
#        tar -xf gdb-14.2.tar.xz
#	    pushd gdb-14.2
#            ./configure --prefix=$BUILD_DEST/gdb --with-isl=$BUILD_DEST/isl
#	        make -j $NUM_THREADS
#		make install
#	    popd
#    popd
#fi

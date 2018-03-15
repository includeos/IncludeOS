#! /bin/bash
. $INCLUDEOS_SRC/etc/set_traps.sh

# Download, configure, compile and install llvm
ARCH=${ARCH:-x86_64} # CPU architecture. Alternatively x86_64
TARGET=$ARCH-elf	# Configure target based on arch. Always ELF.
BUILD_DIR=${BUILD_DIR:-~/IncludeOS_build/build_llvm}
INCLUDEOS_THREADING=${INCLUDEOS_THREADING:-OFF}

musl_inc=$TEMP_INSTALL_DIR/$TARGET/include	# path for newlib headers
IncludeOS_posix=$INCLUDEOS_SRC/api/posix
libcxx_inc=$BUILD_DIR/llvm/projects/libcxx/include
libcxxabi_inc=$BUILD_DIR/llvm/projects/libcxxabi/include
threading=${INCLUDEOS_THREADING:-OFF}

# Install dependencies
sudo apt-get install -y ninja-build zlib1g-dev libtinfo-dev

cd $BUILD_DIR
download_llvm=${download_llvm:-"1"}	# This should be more dynamic
if [ ! -z $download_llvm ]; then
  # Clone LLVM
  echo "Downloading LLVM"
    git clone -b $llvm_branch https://github.com/llvm-mirror/llvm.git || true
    #svn co http://llvm.org/svn/llvm-project/llvm/tags/$LLVM_TAG llvm

    # Clone libc++, libc++abi, and some extra stuff (recommended / required for clang)
    pushd llvm/projects
    git checkout $llvm_branch

    # Compiler-rt
    # git clone -b $llvm_branch https://github.com/llvm-mirror/compiler-rt.git || true
    #svn co http://llvm.org/svn/llvm-project/compiler-rt/tags/$LLVM_TAG compiler-rt

    # libc++abi
    git clone -b $llvm_branch https://github.com/llvm-mirror/libcxxabi.git || true
    #svn co http://llvm.org/svn/llvm-project/libcxxabi/tags/$LLVM_TAG libcxxabi

    # libc++
    git clone -b $llvm_branch https://github.com/llvm-mirror/libcxx.git || true
    #svn co http://llvm.org/svn/llvm-project/libcxx/tags/$LLVM_TAG libcxx

    # libunwind
    git clone -b $llvm_branch https://github.com/llvm-mirror/libunwind.git || true
    #svn co http://llvm.org/svn/llvm-project/libunwind/tags/$LLVM_TAG libunwind

    # Back to start
    popd
fi

if [ -d build_llvm ]; then
  echo -e "\n\n >>> Cleaning previous build \n"
  rm -rf build_llvm
fi

echo -e "\n\n >>> Building libc++ for ${ARCH} \n"

# Make a build-directory
mkdir -p build_llvm
pushd build_llvm

if [ ! -z $clear_llvm_build_cache ]; then
    rm CMakeCache.txt
fi



TRIPLE=$ARCH-pc-linux-elf
CXX_FLAGS="-std=c++14 -msse3 -g -mfpmath=sse -nostdlibinc -D_LIBCPP_HAS_MUSL_LIBC"

# CMAKE configure step
#
# Include-path ordering:
# 1. IncludeOS_posix has to come first, as it provides lots of C11 prototypes that libc++ relies on, but which newlib does not provide (see our math.h)
# 2. libcxx_inc must come before newlib, due to math.h function wrappers around C99 macros (signbit, nan etc)
# 3. newlib_inc provodes standard C headers

echo "Building LLVM for $TRIPLE"

cmake -GNinja $OPTS  \
      -DCMAKE_CXX_FLAGS="$CXX_FLAGS -I$IncludeOS_posix -I$libcxxabi_inc -I$libcxx_inc -I$musl_inc " $BUILD_DIR/llvm \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_INSTALL_PREFIX=$BUILD_DIR/IncludeOS_TEMP_install \
      -DBUILD_SHARED_LIBS=OFF \
      -DTARGET_TRIPLE=$TRIPLE \
      -DLLVM_INCLUDE_TESTS=OFF \
      -DLLVM_ENABLE_THREADS=$threading \
      -DLLVM_DEFAULT_TARGET_TRIPLE=$TRIPLE \
      -DLIBCXX_ENABLE_STATIC=ON \
      -DLIBCXX_ENABLE_SHARED=OFF \
      -DLIBCXX_ENABLE_THREADS=$threading \
      -DLIBCXX_TARGET_TRIPLE=$TRIPLE \
      -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON \
      -DLIBCXX_CXX_ABI=libcxxabi \
      -DLIBCXXABI_TARGET_TRIPLE=$TRIPLE \
      -DLIBCXXABI_ENABLE_THREADS=$threading \
      -DLIBCXXABI_HAS_PTHREAD_API=$threading \
      -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
      -DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
      -DLIBCXX_CXX_ABI_LIBRARY_PATH=$BUILD_DIR/build_llvm/lib/ \
      -DLIBUNWIND_TARGET_TRIPLE=$TRIPLE \
      -DLIBUNWIND_ENABLE_SHARED=OFF

# MAKE
ninja libunwind.a
ninja libc++abi.a
ninja libc++.a

popd

trap - EXIT

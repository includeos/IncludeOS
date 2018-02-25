#! /bin/bash
. $INCLUDEOS_SRC/etc/set_traps.sh

# Download, configure, compile and install llvm
ARCH=${ARCH:-x86_64} # CPU architecture. Alternatively x86_64
TARGET=$ARCH-elf	# Configure target based on arch. Always ELF.
INCLUDEOS_THREADING=${INCLUDEOS_THREADING:-ON}

newlib_inc=$TEMP_INSTALL_DIR/$TARGET/include	# path for newlib headers
IncludeOS_posix=$INCLUDEOS_SRC/api/posix
libcxx_inc=$BUILD_DIR/llvm/projects/libcxx/include
libcxxabi_inc=$BUILD_DIR/llvm/projects/libcxxabi/include


# Install dependencies
sudo apt-get install -y ninja-build zlib1g-dev libtinfo-dev

cd $BUILD_DIR

download_llvm=${download_llvm:-"1"}	# This should be more dynamic

if [ ! -z $download_llvm ]; then
    # Clone LLVM
    git clone -b $llvm_branch git@github.com:llvm-mirror/llvm.git || true
    #svn co http://llvm.org/svn/llvm-project/llvm/tags/$LLVM_TAG llvm

    # Clone libc++, libc++abi, and some extra stuff (recommended / required for clang)
    pushd llvm/projects
    git checkout $llvm_branch

    # Compiler-rt
    git clone -b $llvm_branch git@github.com:llvm-mirror/compiler-rt.git || true
    #svn co http://llvm.org/svn/llvm-project/compiler-rt/tags/$LLVM_TAG compiler-rt

    # libc++abi
    git clone -b $llvm_branch git@github.com:llvm-mirror/libcxxabi.git || true
    #svn co http://llvm.org/svn/llvm-project/libcxxabi/tags/$LLVM_TAG libcxxabi

    # libc++
    git clone -b $llvm_branch git@github.com:llvm-mirror/libcxx.git || true
    #svn co http://llvm.org/svn/llvm-project/libcxx/tags/$LLVM_TAG libcxx

    # libunwind
    git clone -b $llvm_branch git@github.com:llvm-mirror/libunwind.git || true
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
CXX_FLAGS="-std=c++14 -msse3 -mfpmath=sse"

# CMAKE configure step
#
# Include-path ordering:
# 1. IncludeOS_posix has to come first, as it provides lots of C11 prototypes that libc++ relies on, but which newlib does not provide (see our math.h)
# 2. libcxx_inc must come before newlib, due to math.h function wrappers around C99 macros (signbit, nan etc)
# 3. newlib_inc provodes standard C headers

echo "Building LLVM for $TRIPLE"

generator="-GNinja"
cmake  $generator  $BUILD_DIR/llvm/  \
       -DCMAKE_CXX_FLAGS="$CXX_FLAGS -I$IncludeOS_posix -I$libcxxabi_inc -I$libcxx_inc -I$newlib_inc " \
       -DLLVM_PATH=$BUILD_DIR/llvm       \
       -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
       -DBUILD_SHARED_LIBS=OFF \
       -DCMAKE_C_COMPILER=clang-$clang_version \
       -DCMAKE_CXX_COMPILER=clang++-$clang_version \
       -DTARGET_TRIPLE=$TRIPLE \
       -DLLVM_BUILD_32_BITS=OFF \
       -DLLVM_INCLUDE_TESTS=OFF \
       -DLLVM_ENABLE_THREADS=$INCLUDEOS_THREADING \
       -DLLVM_ENABLE_SHARED=OFF \
       -DLLVM_DEFAULT_TARGET_TRIPLE=$TRIPLE \
       -DLIBCXX_ENABLE_SHARED=OFF \
       -DLIBCXX_ENABLE_THREADS=$INCLUDEOS_THREADING \
       -DLIBCXX_TARGET_TRIPLE=$TRIPLE \
       -DLIBCXX_BUILD_32_BITS=OFF \
       -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON \
       -DLIBCXX_CXX_ABI=libcxxabi \
       -DLIBCXXABI_USE_LLVM_UNWINDER=OFF \
       -DLIBUNWIND_ENABLE_SHARED=OFF \
       -DLIBUNWIND_ENABLE_STATIC=ON \
       -DLIBUNWIND_TARGET_TRIPLE=$TRIPLE \
       -DLIBCXXABI_TARGET_TRIPLE=$TRIPLE \
       -DLIBCXXABI_ENABLE_THREADS=$INCLUDEOS_THREADING \
       -DLIBCXXABI_HAS_PTHREAD_API=ON \
       -DLIBCXX_CXX_ABI_LIBRARY_PATH=$BUILD_DIR/build_llvm/lib/ \

# MAKE
# ninja libunwind.a
ninja libc++abi.a
ninja libc++.a

popd

trap - EXIT

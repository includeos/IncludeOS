#! /bin/bash
. $INCLUDEOS_SRC/etc/set_traps.sh

# Download, configure, compile and install llvm

newlib_inc=$TEMP_INSTALL_DIR/i686-elf/include	# path for newlib headers
IncludeOS_posix=$INCLUDEOS_SRC/api/posix
libcxx_inc=$BUILD_DIR/$llvm_src/projects/libcxx/include

# Install dependencies
sudo apt-get install -y cmake ninja-build subversion zlib1g-dev libtinfo-dev

cd $BUILD_DIR

if [ ! -z $download_llvm ]; then
    # Clone LLVM
    svn co http://llvm.org/svn/llvm-project/llvm/tags/$LLVM_TAG llvm

    # Clone libc++, libc++abi, and some extra stuff (recommended / required for clang)
    cd llvm/projects

    # Compiler-rt
    svn co http://llvm.org/svn/llvm-project/compiler-rt/tags/$LLVM_TAG compiler-rt
    # git clone http://llvm.org/git/llvm compiler-rt

    # libc++abi
    svn co http://llvm.org/svn/llvm-project/libcxxabi/tags/$LLVM_TAG libcxxabi
    # git clone http://llvm.org/git/libcxxabi

    # libc++
    svn co http://llvm.org/svn/llvm-project/libcxx/tags/$LLVM_TAG libcxx
    # git clone http://llvm.org/git/libcxx

    # libunwind
    svn co http://llvm.org/svn/llvm-project/libunwind/tags/$LLVM_TAG libunwind
    #git clone http://llvm.org/git/libunwind

    # Back to start
	cd $BUILD_DIR
fi

# Make a build-directory
mkdir -p build_llvm
pushd build_llvm

if [ ! -z $clear_llvm_build_cache ]; then
    rm CMakeCache.txt
fi

# General options
OPTS=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" "

# LLVM General options
OPTS+=-DBUILD_SHARED_LIBS=OFF" "
OPTS+=-DCMAKE_BUILD_TYPE=MinSizeRel" "

# Can't build libc++ with g++ unless it's a cross compiler (need to specify target)
OPTS+=-DCMAKE_C_COMPILER=clang-$clang_version" "
OPTS+=-DCMAKE_CXX_COMPILER=clang++-$clang_version" " # -std=c++11" "

TRIPLE=i686-pc-none-elf

OPTS+=-DTARGET_TRIPLE=$TRIPLE" "
OPTS+=-DLLVM_BUILD_32_BITS=ON" "
OPTS+=-DLLVM_INCLUDE_TESTS=OFF" "
OPTS+=-DLLVM_ENABLE_THREADS=OFF" "
OPTS+=-DLLVM_DEFAULT_TARGET_TRIPLE=$TRIPLE" "

# libc++-specific options
OPTS+=-DLIBCXX_ENABLE_SHARED=OFF" "
OPTS+=-DLIBCXX_ENABLE_THREADS=OFF" "
OPTS+=-DLIBCXX_TARGET_TRIPLE=$TRIPLE" "
OPTS+=-DLIBCXX_BUILD_32_BITS=ON" "

OPTS+=-DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON" "

OPTS+=-DLIBCXX_CXX_ABI=libcxxabi" "
OPTS+=-DLIBCXX_CXX_ABI_INCLUDE_PATHS=$INCLUDEOS_SRC/src/include" "

# libunwind-specific options
OPTS+=-DLIBUNWIND_ENABLE_SHARED=OFF" "
OPTS+=-LIBCXXABI_USE_LLVM_UNWINDER=ON" "

echo "LLVM CMake Build options:" $OPTS

# CXX flags
CXX_FLAGS="-std=c++14 -nostdlibinc -mavx -maes -mfma"

# CMAKE configure step
#
# Include-path ordering:
# 1. IncludeOS_posix has to come first, as it provides lots of C11 prototypes that libc++ relies on, but which newlib does not provide (see our math.h)
# 2. libcxx_inc must come before newlib, due to math.h function wrappers around C99 macros (signbit, nan etc)
# 3. newlib_inc provodes standard C headers
cmake -GNinja $OPTS  -DCMAKE_CXX_FLAGS="$CXX_FLAGS -I$IncludeOS_posix -I$libcxx_inc -I$newlib_inc" $BUILD_DIR/llvm

# MAKE
ninja libc++.a

popd

trap - EXIT

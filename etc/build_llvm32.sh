llvm=llvm
llvm_build=llvm_build

if [ ! -z $install_llvm_dependencies ]; then
    # Dependencies
    sudo apt-get install cmake ninja-build subversion zlib1g-dev:i386 libtinfo-dev:i386
fi

if [ ! -z $download_llvm ]; then
    # Clone LLVM   
    svn co http://llvm.org/svn/llvm-project/llvm/trunk $llvm
    # git clone http://llvm.org/git/llvm 
    
    # Clone CLANG - not necessary to build only libc++ and libc++abi
    # cd llvm/tools              
    # svn co http://llvm.org/svn/llvm-project/cfe/trunk clang
    # svn co http://llvm.org/svn/llvm-project/clang-tools-extra/trunk extra
    
    # Clone libc++, libc++abi, and some extra stuff (recommended / required for clang)
    cd $llvm/projects
    
    # Compiler-rt
    svn co http://llvm.org/svn/llvm-project/compiler-rt/trunk compiler-rt
    # git clone http://llvm.org/git/llvm compiler-rt
    
    # libc++abi
    svn co http://llvm.org/svn/llvm-project/libcxxabi/trunk libcxxabi
    # git clone http://llvm.org/git/libcxxabi
    
    # libc++
    svn co http://llvm.org/svn/llvm-project/libcxx/trunk libcxx
    # git clone http://llvm.org/git/libcxx
    
    # libunwind
    svn co http://llvm.org/svn/llvm-project/libunwind/trunk libunwind
    #git clone http://llvm.org/git/libunwind
    
    # Back to start
    cd ../../
fi

# Make a build-directory
mkdir -p $llvm_build
cd $llvm_build

if [ ! -z $clear_llvm_build_cache ]; then
    rm CMakeCache.txt
fi

# General options
OPTS=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" "

# LLVM General options
OPTS+=-DBUILD_SHARED_LIBS=OFF" "
OPTS+=-DCMAKE_BUILD_TYPE=MinSizeRel" "
#OPTS+=-DCMAKE_BUILD_TYPE=Release" "

# Can't build libc++ with g++ unless it's a cross compiler (need to specify target)
OPTS+=-DCMAKE_C_COMPILER=clang" "
OPTS+=-DCMAKE_CXX_COMPILER=clang++" " # -std=c++11" "

#
# WARNING: It seems imossible to pass in cxx-flags like this; I've tried \' \\" \\\" etc.
#
# OPTS+="-DCMAKE_CXX_FLAGS='-I/home/alfred/IncludeOS/stdlib/support -I/usr/local/IncludeOS/i686-elf/include -I/home/alfred/IncludeOS/stdlib/support/newlib -I/home/alfred/IncludeOS/src/include' " 
# OPTS+='-DCMAKE_CXX_FLAGS=-I/home/alfred/IncludeOS/stdlib/support -I/usr/local/IncludeOS/i686-elf/include  -I/home/alfred/IncludeOS/stdlib/support/newlib -I/home/alfred/IncludeOS/src/include '

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

# OPTS+=-DLIBCXX_GCC_TOOLCHAIN=/usr/local/IncludeOS/i686/" "
OPTS+=-DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON" "

OPTS+=-DLIBCXX_CXX_ABI=libcxxabi" "
OPTS+=-DLIBCXX_CXX_ABI_INCLUDE_PATHS=/home/gonzo/github/IncludeOS/src/include" "


# libunwind-specific options
OPTS+=-DLIBUNWIND_ENABLE_SHARED=OFF" "
OPTS+=-LIBCXXABI_USE_LLVM_UNWINDER=ON" "

echo "LLVM CMake Build options:" $OPTS

IncludeOS_repo=$HOME/IncludeOS
IncludeOS_cstdlib=/usr/local/IncludeOS/i686-elf/include

#
# WARNING: The following will cause the "requires std::atomic" error. 
# (For some reason - the headers should be the same as in llbm/projects/libcxx/include - our mods causes this?"
#
# -I/$IncludeOS_Source/IncludeOS/stdlib

# Various search-path stuff
# -nostdinc -nostdlib -nodefaultlibs -ffreestanding -isystem /usr/local/IncludeOS/i686/ --sysroot=/usr/local/IncludeOS/i686


#
# CONFIGURE
#
# Using makefiles
# time cmake -G"Unix Makefiles" $OPTS  ../$llvm

# Using Ninja (slightly faster, but not by much)
cmake -GNinja $OPTS -DCMAKE_CXX_FLAGS="-std=c++11 -I$IncludeOS_cstdlib -I$IncludeOS_repo/src/include/ -I$IncludeOS_repo/abi -I$IncludeOS_repo/stdlib/support/newlib/  " ../$llvm #  -DCMAKE_CXX_COMPILER='clang++ -std=c++11


#
# MAKE 
#
# Using ninja
#
# ninja libc++abi.a
  ninja libc++.a
# ninja libunwind.a
# ninja compiler-rt

#
# MAKE
#
# Using makefiles
# make cxx
# make cxxabi
# unwind

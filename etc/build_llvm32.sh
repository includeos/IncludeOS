# No shared libs - just .a
# ...Not for libcxx either
# Could use higher, but this is current high from ubuntu repo
# Building for 32-bit


OPTS=-DBUILD_SHARED_LIBS=OFF" "
OPTS+=-DLIBCXX_ENABLE_SHARED=OFF" "
OPTS+=-DCMAKE_BUILD_TYPE=MinSizeRel" "
OPTS+=-DCMAKE_C_COMPILER=clang" "
OPTS+=-DCMAKE_CXX_COMPILER=clang++" "
OPTS+=-DLIBCXX_ENABLE_THREADS=OFF" "
OPTS+=-DTARGET_TRIPLE="i686-elf "
#OPTS+=-DCMAKE_CXX_FLAGS=-target" "
OPTS+=-DLLVM_BUILD_32_BITS=ON" "
OPTS+=-DLIBCXX_BUILD_32_BITS=ON" "
OPTS+=-DCMAKE_CXX_FLAGS=-O2" "
OPTS+=-D_NEWLIB_VERSION=1" "

# Libunwind!


# General options
OPTS+=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" "

echo "OPTIONS:" $OPTS

cmake -G"Unix Makefiles" $OPTS  ../llvm 


make cxx
make cxxabi
make libunwind

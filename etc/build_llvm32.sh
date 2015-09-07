$llvm = llvmes
$llvm_build = llvm_build

# Dependencies
sudo apt-get install cmake subversion zlib1g-dev:i386 libtinfo-dev:i386

# On 64-bit:                                                                                                                      
zlib1g-dev:i386 libtinfo-dev:i386

# Clone LLVM                                                                                                                      
svn co http://llvm.org/svn/llvm-project/llvm/trunk $llvm

# Clone CLANG - not necessary to build only libc++ and libc++abi                                                                  
# cd llvm/tools                                                                                                                   
# svn co http://llvm.org/svn/llvm-project/cfe/trunk clang                                                                         
# svn co http://llvm.org/svn/llvm-project/clang-tools-extra/trunk extra                                                           

# Clone libc++, libc++abi, and some extra stuff (recommended / required for clang)                                                
cd $llvm/projects

svn co http://llvm.org/svn/llvm-project/compiler-rt/trunk compiler-rt
svn co http://llvm.org/svn/llvm-project/libcxxabi/trunk libcxxabi
svn co http://llvm.org/svn/llvm-project/libcxx/trunk libcxx
svn co http://llvm.org/svn/llvm-project/libunwind/trunk libunwind

# Make a build-directory                                                                                                          
cd
mkdir $llvm_build
cd $llvm_build

OPTS=-DBUILD_SHARED_LIBS=OFF" "
OPTS+=-DLIBCXX_ENABLE_SHARED=OFF" "
OPTS+=-DCMAKE_BUILD_TYPE=MinSizeRel" "
OPTS+=-DCMAKE_C_COMPILER=clang" "
OPTS+=-DCMAKE_CXX_COMPILER=clang++" "
OPTS+=-DLIBCXX_ENABLE_THREADS=OFF" "
OPTS+=-DTARGET_TRIPLE="i686-elf "
OPTS+=-DLLVM_BUILD_32_BITS=ON" "
OPTS+=-DLIBCXX_BUILD_32_BITS=ON" "

# TODO: Libunwind! There's an option in the CMakeLists.txt of libcxx, specifying to use llvm's libunwind
# TODO: How to enable newlib support : set _NEWLIB_VERSION=1 ? 

# General options
OPTS+=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" "

echo "LLVM CMake Build options:" $OPTS

cmake -G"Unix Makefiles" $OPTS  ../$llvm

make cxx
make cxxabi
make libunwind

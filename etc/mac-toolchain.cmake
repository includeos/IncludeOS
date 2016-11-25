# This is not an actual crosscompile toolchain
# but it do make it possible to compile on Mac with
#
# "cmake -DCMAKE_TOOLCHAIN_FILE=mac-toolchain.cmake"
#
# It's based on the environment setup by /etc/install_osx.sh

# Compile target (triple)
set(target i686-elf)

# Set compiler to clang 3.8
# This assumes versioned clang in /usr/local/bin
# (llvm38 installed by brew)
set(CMAKE_C_COMPILER /usr/local/bin/clang-3.8)
set(CMAKE_CXX_COMPILER /usr/local/bin/clang++-3.8)

#set(CMAKE_C_COMPILER_TARGET ${target})
#set(CMAKE_CXX_COMPILER_TARGET ${target})

# This is a hack (I think?), but it lets CMake
# find cross compile tools prefixed with the triple
# in the same folder as the compiler is located.
# e.g. /usr/local/bin/i686-elf-<EXECUTABLE>
#
# This can be done with /etc/install_binutils.sh
set(_CMAKE_TOOLCHAIN_PREFIX ${target}-)

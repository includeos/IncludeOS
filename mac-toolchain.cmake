# This is not an actual crosscompile toolchain
# but it do make it possible to compile on Mac with
# "cmake -DCMAKE_TOOLCHAIN_FILE=mac-toolchain.cmake"

# Compile target (triple)
set(triple i686-pc-none-elf)

# Set compiler to clang (installed by brew)
set(CMAKE_C_COMPILER /usr/local/bin/clang-3.8)
set(CMAKE_CXX_COMPILER /usr/local/bin/clang++-3.8)

#set(CMAKE_C_COMPILER_TARGET ${triple})
#set(CMAKE_CXX_COMPILER_TARGET ${triple})

# This is a hack (I think?), but it lets CMake find our
# binutils tools installed along side the compiler in
# "/usr/local/bin/i686-pc-none-elf"
set(_CMAKE_TOOLCHAIN_PREFIX ${triple}/bin/)

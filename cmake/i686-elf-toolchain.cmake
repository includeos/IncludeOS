if(APPLE) # Only use toolchain for macaroni
# This is not an actual crosscompile toolchain
# but it do make it possible to compile on Mac with
#
# It's based on the environment setup by /etc/install_osx.sh
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(CMAKE_OSX_ARCHITECTURES i686) # Does not help at all
set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "i686")

# Bin directory
set(INCLUDEOS_BIN $ENV{INCLUDEOS_PREFIX}/includeos/bin)

# Compile target (triple)
set(TARGET i686-elf)

# Set compiler to the one in includeos/bin (clang)
set(CMAKE_C_COMPILER ${INCLUDEOS_BIN}/gcc)
set(CMAKE_CXX_COMPILER ${INCLUDEOS_BIN}/g++)

set(CMAKE_C_COMPILER_TARGET ${TARGET})
set(CMAKE_CXX_COMPILER_TARGET ${TARGET})

# Compiler test won't pass on Mac due to wrong target
# being passed when linked
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_FIND_ROOT_PATH ${INCLUDEOS_BIN})

# This is a hack (I think?), but it lets CMake
# find cross compile tools prefixed with the triple
# in the same folder as the compiler is located.
# e.g. /usr/local/bin/i686-elf-<EXECUTABLE>
#
# This can be done with /etc/install_binutils.sh
#set(_CMAKE_TOOLCHAIN_PREFIX ${target}-)

# Set nasm compiler to the one symlinked in includeos/bin (to avoid running Mac one)
set(CMAKE_ASM_NASM_COMPILER ${INCLUDEOS_BIN}/nasm)
endif()

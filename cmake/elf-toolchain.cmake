if(APPLE) # Only use toolchain for macaroni
# This is not a full-fledged crosscompile toolchain. In particular we're not using
# sysroot, which means the default C / C++ compiler tests won't work as it doesn't
# know about the IncludeOS include- and link paths. These are specified manually in
# the main CMakeLists and service specific CmakeLists respectively.
#
# NOTE: CMake uses the compiler as front-end for finding the linker and other
# binutils. Clang expects these to be named according to the target triple, e.g.
# <your-target-triple>-ld, -ar, -objcopy etc., and it will look for them in the
# <toolchain>/bin directory. The binutils installation should have
# made these available in $INCLUDEOS_PREFIX/includeos/bin.

# INCLUDEOS_PREFIX is based on the environment setup by /etc/install_osx.sh
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(CMAKE_SYSTEM_NAME "Linux")
set(CMAKE_SYSTEM_PROCESSOR "x86_64")

set(TRIPLE "${CMAKE_SYSTEM_PROCESSOR}-pc-linux-elf")
set(CMAKE_CXX_COMPILER_TARGET ${TRIPLE})
set(CMAKE_C_COMPILER_TARGET ${TRIPLE})

message(STATUS "Target triple ${TRIPLE}")

# Bin directory
set(INCLUDEOS_BIN $ENV{INCLUDEOS_PREFIX}/bin)

# Set compiler to the one in includeos/bin (clang)
set(CMAKE_C_COMPILER ${INCLUDEOS_BIN}/gcc)
set(CMAKE_CXX_COMPILER ${INCLUDEOS_BIN}/g++)

# Tell cmake where to look for binutils
set(CMAKE_FIND_ROOT_PATH ${INCLUDEOS_BIN})

# The root path is not complete with includes etc. (passed in manually)
# so the default cmake compiler tests won't be able to produce a complete binary.
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# Set nasm compiler to the one symlinked in includeos/bin (to avoid running Mac one)
set(CMAKE_ASM_NASM_COMPILER ${INCLUDEOS_BIN}/nasm)

# Disable solo5 handled by cmake dependent options in main CMakelists.txt
endif()

# Target CPU Architecture
if (NOT DEFINED ARCH)
  if (DEFINED ENV{ARCH})
    set(ARCH $ENV{ARCH})
  else()
    set(ARCH x86_64)
  endif()
endif()

if (NOT DEFINED PLATFORM)
  if (DEFINED ENV{PLATFORM})
    set(PLATFORM $ENV{PLATFORM})
  else()
    set(PLATFORM x86_pc)
  endif()
endif()


message(STATUS "Building for arch ${ARCH}, platform ${PLATFORM}")

set(TRIPLE ${ARCH}) #-pc-linux-elf
set(DCMAKE_CXX_COMPILER_TARGET ${TRIPLE})
set(DCMAKE_C_COMPILER_TARGET ${TRIPLE})

option(single_threaded "Compile without SMP support" ON)

# include toolchain for arch
include($ENV{INCLUDEOS_PREFIX}/includeos/${ARCH}-elf-toolchain.cmake)

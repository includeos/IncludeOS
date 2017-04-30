# Target CPU Architecture
if (NOT DEFINED ARCH)
  if (DEFINED ENV{ARCH})
    set(ARCH $ENV{ARCH})
  else()
    set(ARCH x86_64)
  endif()
endif()

message(STATUS "Building for arch ${ARCH}")

set(TRIPLE ${ARCH}) #-pc-linux-elf
set(DCMAKE_CXX_COMPILER_TARGET ${TRIPLE})
set(DCMAKE_C_COMPILER_TARGET ${TRIPLE})

# include toolchain for arch
include($ENV{INCLUDEOS_PREFIX}/includeos/${ARCH}-elf-toolchain.cmake)

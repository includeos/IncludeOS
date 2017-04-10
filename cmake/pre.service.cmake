# Target CPU Architecture
set(ARCH i686)
if(DEFINED ENV{ARCH})
  set(ARCH $ENV{ARCH})
endif()

set(TRIPLE ${ARCH}) #-pc-linux-elf
set(DCMAKE_CXX_COMPILER_TARGET ${TRIPLE})
set(DCMAKE_C_COMPILER_TARGET ${TRIPLE})

# include toolchain for arch
include($ENV{INCLUDEOS_PREFIX}/includeos/${ARCH}-elf-toolchain.cmake)

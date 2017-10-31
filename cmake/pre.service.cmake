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

# configure options
option(debug "Build with debugging symbols (OBS: increases binary size)" OFF)
option(minimal "Build for minimal size" OFF)
option(stripped "Strip symbols to further reduce size" OFF)
option(threading "Compile threading and SMP support" OFF)
option (undefined_san "Enable undefined-behavior sanitizer" OFF)

# arch and platform defines
message(STATUS "Building for arch ${ARCH}, platform ${PLATFORM}")
set(TRIPLE ${ARCH}) #-pc-linux-elf
set(DCMAKE_CXX_COMPILER_TARGET ${TRIPLE})
set(DCMAKE_C_COMPILER_TARGET ${TRIPLE})

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
add_definitions(-DPLATFORM="${PLATFORM}")
add_definitions(-DPLATFORM_${PLATFORM})

if (NOT threading)
  add_definitions(-DINCLUDEOS_SINGLE_THREADED)
  add_definitions(-D_LIBCPP_HAS_NO_THREADS)
endif()

# include toolchain for arch
include($ENV{INCLUDEOS_PREFIX}/includeos/${ARCH}-elf-toolchain.cmake)

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
option(default_stdout "Use the OS default stdout (serial)" ON)

option(debug "Build with debugging symbols (OBS: increases binary size)" OFF)
option(minimal "Build for minimal size" OFF)
option(stripped "Strip symbols to further reduce size" OFF)

option(smp "Enable SMP (multiprocessing)" OFF)
option(undefined_san "Enable undefined-behavior sanitizer" OFF)
option(thin_lto "Enable Thin LTO plugin" OFF)
option(full_lto "Enable full LTO (also works on LD)" OFF)
option(coroutines "Compile with coroutines TS support" OFF)

# arch and platform defines
message(STATUS "Building for arch ${ARCH}, platform ${PLATFORM}")
set(TRIPLE "${ARCH}-pc-linux-elf")
set(CMAKE_CXX_COMPILER_TARGET ${TRIPLE})
set(CMAKE_C_COMPILER_TARGET ${TRIPLE})

set(CPP_VERSION c++17)

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
add_definitions(-DPLATFORM="${PLATFORM}")
add_definitions(-DPLATFORM_${PLATFORM})

# include toolchain for arch only for macaroni
include($ENV{INCLUDEOS_PREFIX}/cmake/elf-toolchain.cmake)

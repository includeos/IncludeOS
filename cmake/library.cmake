#
# CMakeList for IncludeOS library
#

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(INSTALL_LOC $ENV{INCLUDEOS_PREFIX}/includeos)

# TODO: Verify that the OS libraries exist
set(ARCH x86_64)
if(DEFINED ENV{ARCH})
  set(ARCH $ENV{ARCH})
endif()
message(STATUS "Target CPU architecture ${ARCH}")

set(TRIPLE "${ARCH}-pc-linux-elf")
set(CMAKE_CXX_COMPILER_TARGET ${TRIPLE})
set(CMAKE_C_COMPILER_TARGET ${TRIPLE})
message(STATUS "Target triple ${TRIPLE}")

# Assembler
if ("${ARCH}" STREQUAL "x86_64")
  set (ARCH_INTERNAL "ARCH_X64")
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf64")
else()
  set (ARCH_INTERNAL "ARCH_X86")
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf")
endif()
enable_language(ASM_NASM)

# defines $CAPABS depending on installation
include(${CMAKE_CURRENT_LIST_DIR}/settings.cmake)

# Various global defines
# * OS_TERMINATE_ON_CONTRACT_VIOLATION provides classic assert-like output from Expects / Ensures
# * _GNU_SOURCE enables POSIX-extensions in newlib, such as strnlen. ("everything newlib has", ref. cdefs.h)
set(CAPABS "${CAPABS} -fstack-protector-strong -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE")
set(WARNS  "-Wall -Wextra") #-pedantic

# configure options
option(debug "Build with debugging symbols (OBS: increases binary size)" OFF)
option(minimal "Build for minimal size" OFF)
option(stripped "reduce size" OFF)

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

set(OPTIMIZE "-O2")
if (minimal)
  set(OPTIMIZE "-Os")
endif()
if (debug)
  set(CAPABS "${CAPABS} -g")
endif()

if (CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_CXX_FLAGS "-m32 -MMD ${CAPABS} ${WARNS} -nostdlib -fno-omit-frame-pointer -c -std=c++20 -D_LIBCPP_HAS_NO_THREADS=1")
  set(CMAKE_C_FLAGS "-m32 -MMD ${CAPABS} ${WARNS} -nostdlib -fno-omit-frame-pointer -c")
else()
  # these kinda work with llvm
  set(CMAKE_CXX_FLAGS "-MMD ${CAPABS} ${OPTIMIZE} ${WARNS} -nostdlib -nostdlibinc -fno-omit-frame-pointer -c -std=c++20 -fno-threadsafe-statics -D_LIBCPP_HAS_NO_THREADS=1")
  set(CMAKE_C_FLAGS "-MMD ${CAPABS} ${OPTIMIZE} ${WARNS} -nostdlib -nostdlibinc -fno-omit-frame-pointer -c")
endif()

# includes
include_directories(${LOCAL_INCLUDES})
include_directories(${INSTALL_LOC}/api/posix)
include_directories(${INSTALL_LOC}/${ARCH}/include/libcxx)
include_directories(${INSTALL_LOC}/${ARCH}/include/newlib)
include_directories(${INSTALL_LOC}/${ARCH}/include/solo5)
include_directories(${INSTALL_LOC}/api)
include_directories(${INSTALL_LOC}/include)
include_directories($ENV{INCLUDEOS_PREFIX}/include)

add_library(${LIBRARY_NAME} STATIC ${SOURCES})

#install(TARGETS ${LIBRARY_NAME} DESTINATION includeos/lib)
#install(DIRECTORY ${LIBRARY_HEADERS} DESTINATION includeos/include)

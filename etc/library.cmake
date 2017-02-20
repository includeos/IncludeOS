#
# CMakeList for IncludeOS library
#

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

# test compiler
if(CMAKE_COMPILER_IS_GNUCC)
	# currently gcc is not supported due to problems cross-compiling a unikernel
	# (i.e., building a 32bit unikernel (only supported for now) on a 64bit system)
	message(FATAL_ERROR "GCC is not currently supported, please clean-up build directory and configure for clang through CC and CXX environment variables")
endif(CMAKE_COMPILER_IS_GNUCC)

set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf")
enable_language(ASM_NASM)

# Various global defines
# * OS_TERMINATE_ON_CONTRACT_VIOLATION provides classic assert-like output from Expects / Ensures
# * _GNU_SOURCE enables POSIX-extensions in newlib, such as strnlen. ("everything newlib has", ref. cdefs.h)
set(CAPABS "-msse3 -fstack-protector-strong -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE")
set(WARNS  "-Wall -Wextra") #-pedantic

# configure options
option(debug "Build with debugging symbols (OBS: increases binary size)" OFF)
option(minimal "Build for minimal size" OFF)
option(stripped "reduce size" OFF)

set(OPTIMIZE "-O2")
if (minimal)
  set(OPTIMIZE "-Os")
endif()
if (debug)
  set(CAPABS "${CAPABS} -g")
endif()

# these kinda work with llvm
set(CMAKE_CXX_FLAGS "-MMD -target i686-elf ${CAPABS} ${OPTIMIZE} ${WARNS} -c -m32 -std=c++14 -D_LIBCPP_HAS_NO_THREADS=1")
set(CMAKE_C_FLAGS "-MMD -target i686-elf ${CAPABS} ${OPTIMIZE} ${WARNS} -c -m32")

# includes
include_directories(${LOCAL_INCLUDES})
include_directories($ENV{INCLUDEOS_PREFIX}/includeos/api/posix)
include_directories($ENV{INCLUDEOS_PREFIX}/includeos/include/libcxx)
include_directories($ENV{INCLUDEOS_PREFIX}/includeos/include/newlib)
include_directories($ENV{INCLUDEOS_PREFIX}/includeos/api)
include_directories($ENV{INCLUDEOS_PREFIX}/includeos/include)
include_directories($ENV{INCLUDEOS_PREFIX}/include)

add_library(${LIBRARY_NAME} STATIC ${SOURCES})

#install(TARGETS ${LIBRARY_NAME} DESTINATION includeos/lib)
#install(DIRECTORY ${LIBRARY_HEADERS} DESTINATION includeos/include)

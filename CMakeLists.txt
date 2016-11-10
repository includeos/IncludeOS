cmake_minimum_required(VERSION 2.8.9)

project (includeos)

set(INCLUDEOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

# test compiler
if(CMAKE_COMPILER_IS_GNUCC)
	# currently gcc is not supported due to problems cross-compiling a unikernel
	# (i.e., building a 32bit unikernel (only supported for now) on a 64bit system)
	message(FATAL_ERROR "usage of Gcc is not currently supported, please clean-up build directory and configure for clang through CC and CXX environmental variables")
endif(CMAKE_COMPILER_IS_GNUCC)

# TODO: should this not be moved into the library itself
set(INCLUDEOS_INSTALL_DIR $ENV{HOME}/IncludeOS_install/)
set(INCLUDEOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

# get new stack protector value
execute_process(COMMAND git describe --dirty
	        WORKING_DIRECTORY ${INCLUDEOS_ROOT}
	        OUTPUT_VARIABLE OS_VERSION)
string(STRIP ${OS_VERSION} OS_VERSION)

# get current project version from git
execute_process(COMMAND awk "BEGIN{srand(); print int(rand()*65536)}"
                OUTPUT_VARIABLE STACK_PROTECTOR_VALUE)
string(STRIP ${STACK_PROTECTOR_VALUE} STACK_PROTECTOR_VALUE)

# stackrealign is needed to guarantee 16-byte stack alignment for SSE
# the compiler seems to be really dumb in this regard, creating a misaligned stack left and right
set(CAPABS "-mstackrealign -msse3 -fstack-protector-strong -D_STACK_GUARD_VALUE_=${STACK_PROTECTOR_VALUE} ")

# Various global defines
# * NO_DEBUG Disables  output from the debug macro
# * OS_TERMINATE_ON_CONTRACT_VIOLATION provides classic assert-like output from Expects / Ensures
# * _GNU_SOURCE enables POSIX-extensions in newlib, such as strnlen. ("everything newlib has", ref. cdefs.h)
set(CAPABS "${CAPABS} -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE")

set(WARNS  "-Wall -Wextra") #-pedantic

# configure options
option(debug "Build with debugging symbols (OBS: Dramatically increases binary size)" OFF)
option(debug-info "Build like \"all\" but with debugging output (i.e. the 'debug'-macro) enabled" OFF)
option(debug-all "Build with debugging symbols + debugging output, i.e. \"debug\" + \"debug-info\"" OFF)
option(minimal "Build for minimal size" OFF)
option(stripped "reduce size" OFF)

# set optimization level

# set optimization level

set(OPTIMIZE "-O2")

if(debug OR debug-info OR debug-all)
	set(CAPABS "${CAPABS} -O0")
endif(debug OR debug-info OR debug-all)

if(minimal)
	set(OPTIMIZE "-Os")
endif(minimal)

# Set debug options
if(debug OR debug-all)
	set(CAPABS "${CAPABS} -ggdb3 -DGSL_THROW_ON_CONTRACT_VIOLATION")
endif(debug OR debug-all)

if(debug-info OR debug-all)
	set(CAPABS "${CAPABS} -UNO_DEBUG")
endif(debug-info OR debug-all)

option(silent OFF)
if(silent)
	set(CAPABS "${CAPABS} -DNO-INFO=1")
endif(silent)

# Append optimization level
set(CAPABS "${CAPABS} ${OPTIMIZE}")

# these kinda work with llvm
set(CMAKE_CXX_FLAGS "-MMD -target i686-elf ${CAPABS} ${WARNS} -c -m32 -std=c++14 -D_LIBCPP_HAS_NO_THREADS=1 -DOS_VERSION=\\\"${OS_VERSION}\\\"")
set(CMAKE_C_FLAGS "-MMD -target i686-elf ${CAPABS} ${WARNS} -c -m32 -DOS_VERSION=\"\"${OS_VERSION}\"\"")

# either download or cross-compile needed libraries
option(from_bundle "Download and use pre-compiled libraries for cross-comilation" ON)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cross_compiled_libraries.txt)

add_subdirectory(src)
add_subdirectory(vmbuild)

option(examples "Build example unikernels in /examples" ON)
if(examples)
	add_subdirectory(examples)
endif(examples)

install(DIRECTORY api/ DESTINATION include/api)

enable_testing()
add_subdirectory(test)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/etc/service.cmake DESTINATION share/includeos)

install(DIRECTORY mod/GSL/gsl DESTINATION include/gsl)

set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_PACKAGE_VERSION ${OS_VERSION})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ingve")
include(CPack)

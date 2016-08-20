cmake_minimum_required(VERSION 2.8.9)

project (includeos)

# TODO: should this not be moved into the library itself
set(INCLUDEOS_INSTALL_DIR $ENV{HOME}/IncludeOS_install/)
set(INCLUDEOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

# get new stack protector value
execute_process(COMMAND awk "BEGIN{srand(); print int(rand()*65536)}"
                OUTPUT_VARIABLE STACK_PROTECTOR_VALUE)
string(STRIP ${STACK_PROTECTOR_VALUE} STACK_PROTECTOR_VALUE)

# stackrealign is needed to guarantee 16-byte stack alignment for SSE
# the compiler seems to be really dumb in this regard, creating a misaligned stack left and right
set(COMMON_CAPABS "-mstackrealign -msse3 -fstack-protector-strong -D_STACK_GUARD_VALUE_=${STACK_PROTECTOR_VALUE}")

# Various global defines
# * NO_DEBUG Disables  output from the debug macro
# * OS_TERMINATE_ON_CONTRACT_VIOLATION provides classic assert-like output from Expects / Ensures
# * _GNU_SOURCE enables POSIX-extensions in newlib, such as strnlen. ("everything newlib has", ref. cdefs.h)
set(CAPABS "${COMMON_CAPABS} -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE")
set(WARNS  "-Wall -Wextra") #-pedantic
set(DEBUG_OPTS "-ggdb3")

# these kinda work with llvm
set(CMAKE_CXX_FLAGS "-MMD -target i686-elf ${CAPABS} ${WARNS} -c -m32 -std=c++14 -D_LIBCPP_HAS_NO_THREADS=1 -DOS_VERSION=\"\\\"v0.8.1-709-gf8fccf4\\\"\"")
set(CMAKE_C_FLAGS "-MMD -target i686-elf ${CAPABS} ${WARNS} -c -m32  -D_LIBCPP_HAS_NO_THREADS=1 -DOS_VERSION=\"\\\"v0.8.1-709-gf8fccf4\\\"\"")

# TODO: use right variables
option(debug "Build with debugging symbols (OBS: Dramatically increases binary size)" OFF)
if(debug)
	list(append CAPABS "-O0")
	list(append CMAKE_CXX_FLAGS ${DEBUG_OPTS})
	list(append CPPOPTS "-DGSL_THROW_ON_CONTRACT_VIOLATION")
endif(debug)

add_subdirectory(src)
add_subdirectory(vmbuild)

option(examples "Build example unikernels in /examples" ON)
if(examples)
	add_subdirectory(examples)
endif(examples)

cmake_minimum_required(VERSION 2.8.9)

project (includeos)

# TODO: should this not be moved into the library itself
set(INCLUDEOS_INSTALL_DIR $ENV{HOME}/IncludeOS_install/)
set(INCLUDEOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

# these kinda work with llvm
set(CMAKE_CXX_FLAGS "-MMD -target i386-elf -mstackrealign -msse3 -fstack-protector-strong -D_STACK_GUARD_VALUE_=15583 -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE -Wall -Wextra  -c -m32 -std=c++14 -D_LIBCPP_HAS_NO_THREADS=1 -DOS_VERSION=\"\\\"v0.8.1-709-gf8fccf4\\\"\"")
set(CMAKE_C_FLAGS "-MMD -target i386-elf -mstackrealign -msse3 -fstack-protector-strong -D_STACK_GUARD_VALUE_=15583 -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE -Wall -Wextra  -c -m32  -D_LIBCPP_HAS_NO_THREADS=1 -DOS_VERSION=\"\\\"v0.8.1-709-gf8fccf4\\\"\"")

# TODO: use right variables
option(debug "Build with debugging symbols (OBS: Dramatically increases binary size)" OFF)
if(debug)
	list(append CAPABS "-O0")
	list(append CCOPTS ${DEBUG_OPTS})
	list(append CPPOPTS "-DGSL_THROW_ON_CONTRACT_VIOLATION")
endif(debug)

add_subdirectory(src)
add_subdirectory(vmbuild)
#add_subdirectory(test/lest)

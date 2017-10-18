cmake_minimum_required (VERSION 3.0.2)
project (userland_tests C CXX)

# create OS version string from git describe (used in CXX flags)
execute_process(COMMAND git describe --dirty
	        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
	        OUTPUT_VARIABLE OS_VERSION)
string(STRIP ${OS_VERSION} OS_VERSION)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -march=native -Wall -Wextra")

option(GPROF "Enable profiling with gprof" OFF)
option(SANITIZE "Enable undefined- and address sanitizers" OFF)


if(GPROF)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

if(SANITIZE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fsanitize=address")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOS_VERSION=\\\"${OS_VERSION}\\\"")

add_definitions(-DARCH="x86_64" -DARCH_x86_64)
add_definitions(-DOS_TERMINATE_ON_CONTRACT_VIOLATION)
add_definitions(-DARP_PASSTHROUGH)
add_definitions(-DNO_DEBUG)
add_definitions(-DINCLUDEOS_SINGLE_THREADED)

include_directories(../api)
include_directories(../mod)
include_directories(../mod/GSL)

add_subdirectory(src)
add_subdirectory(userspace)

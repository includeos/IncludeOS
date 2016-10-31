cmake_minimum_required(VERSION 2.8.9)

project (includeos)

set(INCLUDEOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

# test compiler
if(CMAKE_COMPILER_IS_GNUCC)
	# currently gcc is not supported due to problems cross-compiling a unikernel
	# (i.e., building a 32bit unikernel (only supported for now) on a 64bit system)
	message(FATAL_ERROR "usage of Gcc is not currently supported, please clean-up build directory and configure for clang thorugh CC and CXX environmental variables")
endif(CMAKE_COMPILER_IS_GNUCC)

# set copmiler options (performance, warning-level, etc.)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compiler_options.txt)

# either download or cross-compile needed libraries
option(from_bundle "Download and use pre-compiled libraries for cross-comilation" ON)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cross_compiled_libraries.txt)

add_subdirectory(src)
add_subdirectory(vmbuild)

option(examples "Build example unikernels in /examples" ON)
if(examples)
	add_subdirectory(examples)
endif(examples)

install(DIRECTORY api/ DESTINATION include/api
	FILES_MATCHING PATTERN "*.h")

enable_testing()
add_subdirectory(test)

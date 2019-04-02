cmake_minimum_required(VERSION 3.0)
# IncludeOS install location
project(chainloader C CXX)

include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

set(ARCH i686)
include(os)

option(default_stdout "" OFF)
set(SOURCES
    service.cpp hotswap.cpp
  )

os_add_executable(chainloader "IncludeOS chainloader" ${SOURCES})
os_install(TARGETS chainloader DESTINATION bin)

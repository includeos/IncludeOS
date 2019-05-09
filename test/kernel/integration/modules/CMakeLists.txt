cmake_minimum_required(VERSION 3.0)

set(ARCH i686)
set(PLATFORM x86_nano)

project (test_kprint)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

include(os)

MESSAGE(STATUS "CMake root: " ${INCLUDEOS_PREFIX})

set(SOURCES
    service.cpp
  )

os_add_executable(kernel_modules "Kernel modules test" ${SOURCES})

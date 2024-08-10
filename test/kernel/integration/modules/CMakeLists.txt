cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(ARCH i686)
set(PLATFORM x86_nano)

project (service)
include(os)

MESSAGE(STATUS "CMake root: " ${INCLUDEOS_PREFIX})

set(SOURCES
    service.cpp
  )

os_add_executable(kernel_modules "Kernel modules test" ${SOURCES})
configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

cmake_minimum_required(VERSION 3.0)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
# IncludeOS install location
project(chainloader C CXX)

set(ARCH i686)
include(${INCLUDEOS_PACKAGE}/cmake/os.cmake)

option(default_stdout "" OFF)
set(SOURCES
    service.cpp hotswap.cpp
  )
set(LIBRARIES
  ${INCLUDEOS_PACKAGE}/lib/libos.a
  ${INCLUDEOS_PACKAGE}/lib/libarch.a
  ${INCLUDEOS_PACKAGE}/lib/libmusl_syscalls.a
  ${INCLUDEOS_PACKAGE}/platform/libx86_64_pc.a
  ${INCLUDEOS_LIBCXX_PATH}
  ${INCLUDEOS_LIBC_PATH}
  ${INCLUDEOS_LIBUNWIND_PATH}
  ${INCLUDEOS_LIBCXXABI_PATH}
)

os_add_executable(chainloader "IncludeOS chainloader" ${SOURCES})
os_install(TARGETS chainloader DESTINATION bin)

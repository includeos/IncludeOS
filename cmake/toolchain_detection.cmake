cmake_minimum_required(VERSION 2.8.9)

# Use custom toolchain if set
set(toolchain $ENV{INCLUDEOS_TOOLCHAIN_FILE})
if(DEFINED toolchain)
  MESSAGE(STATUS "IncludeOS toolchain file set to: " ${toolchain})
  set(CMAKE_TOOLCHAIN_FILE ${toolchain})
endif(DEFINED toolchain)

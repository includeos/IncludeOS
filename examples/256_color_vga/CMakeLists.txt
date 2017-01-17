cmake_minimum_required(VERSION 2.8.9)
# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
set(CMAKE_TOOLCHAIN_FILE $ENV{INCLUDEOS_PREFIX}/includeos/i686-elf-toolchain.cmake)
project (256_color_vga)

# Human-readable name of your service
set(SERVICE_NAME "256 Color VGA Example Service")

# Name of your service binary
set(BINARY       "256_color_vga_example")

# Maximum memory can be hard-coded into the binary
set(MAX_MEM 128)


# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp int32.asm
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)
set_property(SOURCE service.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -mstackrealign")

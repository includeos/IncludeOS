cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

option(benchmark_mode "Optimizations for benchmarking" OFF)
option(SERIAL_OUTPUT "Output information" OFF)
if (benchmark_mode)
  add_definitions(-DBENCHMARK_MODE)
endif()

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp test_boot.cpp
  )
os_add_executable(kernel_LiveUpdate "LiveUpdate integration test" ${SOURCES})

os_add_drivers(kernel_LiveUpdate
  virtionet
  boot_logger
)

if (SERIAL_OUTPUT)
  os_add_stdout(kernel_LiveUpdate default_stdout)
endif()

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

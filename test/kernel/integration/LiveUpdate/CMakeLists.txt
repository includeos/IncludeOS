cmake_minimum_required(VERSION 3.0)

#service
project (service)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

include(os)

option(benchmark_mode "Optimizations for benchmarking" OFF)
option(SERIAL_OUTPUT "Output information" OFF)
if (benchmark_mode)
  add_definitions(-DBENCHMARK_MODE)
endif()

# Human-readable name of your service
#set(SERVICE_NAME "LiveUpdate integration test")

# Name of your service binary
#set(BINARY       "service")



# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp test_boot.cpp
  )
os_add_executable(kernel_LiveUpdate "LiveUpdate integration test" ${SOURCES})

os_add_drivers(kernel_LiveUpdate
  virtionet
  boot_logger
)

os_add_plugins(kernel_LiveUpdate
  system_log
)

if (SERIAL_OUTPUT)
  os_add_stdout(kernel_LiveUpdate default_stdout)
endif()

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

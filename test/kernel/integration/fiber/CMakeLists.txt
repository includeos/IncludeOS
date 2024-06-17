cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
if (DEFINED ENV{INCLUDEOS_THREADING})
  option(threading "" ENV{INCLUDEOS_THREADING})
endif()

project (service)
include(os)

set(SOURCES
    service.cpp
)
if (threading)
  list(APPEND SOURCES fiber_smp.cpp)
endif()

os_add_executable(kernel_fiber "GRUB boot test" ${SOURCES})
os_add_stdout(kernel_fiber default_stdout)
os_add_drivers(kernel_fiber boot_logger)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

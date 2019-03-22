cmake_minimum_required(VERSION 3.0)
if (DEFINED ENV{INCLUDEOS_THREADING})
  option(threading "" ENV{INCLUDEOS_THREADING})
endif()


#service
project (service)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

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

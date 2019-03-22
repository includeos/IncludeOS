cmake_minimum_required(VERSION 3.0)

# Service
project (pageprot)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

include(os)

set(SOURCES
    service.cpp # ...add more here
)

os_add_executable(kernel_paging "Page protection test" ${SOURCES})
os_add_stdout(kernel_paging default_stdout)
os_add_drivers(kernel_paging boot_logger)
os_add_plugins(kernel_paging vfs)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

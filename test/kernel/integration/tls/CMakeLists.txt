cmake_minimum_required(VERSION 2.8.9)

#option(threading "" ON)

# Service
project (tls)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

include(os)

set(SOURCES
    service.cpp # ...add more here
)

os_add_executable(kernel_tls "TLS test" ${SOURCES})
os_add_stdout(kernel_tls default_stdout)
os_add_drivers(kernel_tls boot_logger)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

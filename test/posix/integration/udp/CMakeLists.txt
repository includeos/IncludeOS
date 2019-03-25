cmake_minimum_required(VERSION 3.0)

#service
project (service)
if (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
  conan_basic_setup()
else()
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
include(os)

set(SOURCES
    service.cpp
  )

#os_add_config(service "${CMAKE_CURRENT_SOURCE_DIR}/config.json")

os_add_executable(posix_udp "POSIX UDP test" ${SOURCES})

os_add_drivers(posix_udp virtionet)
os_add_stdout(posix_udp default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

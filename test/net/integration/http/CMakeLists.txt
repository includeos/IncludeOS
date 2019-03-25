cmake_minimum_required(VERSION 3.0)

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

os_add_executable(net_http "HTTP test" ${SOURCES})

os_add_drivers(net_http virtionet)
os_add_stdout(net_http default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

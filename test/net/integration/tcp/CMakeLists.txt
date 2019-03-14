cmake_minimum_required(VERSION 3.0)

#service
project (service)
if (NOT DEFINED CONAN_DEPENDENCIES)
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
  if (NOT HAS_CONAN)
    message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
  endif()
endif()
conan_basic_setup()

include(os)

set(SOURCES
    service.cpp
  )

os_add_executable(net_tcp "TCP test" ${SOURCES})

os_add_drivers(net_tcp virtionet e1000 vmxnet3)
os_add_stdout(net_tcp default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

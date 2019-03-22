cmake_minimum_required(VERSION 3.0)

#service
project (test_slaac)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

include(os)

os_add_executable(net_slaac "IncludeOS Slaac test" service.cpp)

os_add_drivers(net_slaac virtionet)
os_add_stdout(net_slaac default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

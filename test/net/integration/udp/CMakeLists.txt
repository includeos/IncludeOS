cmake_minimum_required(VERSION 3.0)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
#service
project (service)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

include(os)

os_add_executable(net_udp "UDP test" service.cpp)

os_add_drivers(net_udp virtionet ip4_reassembly)
os_add_stdout(net_udp default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

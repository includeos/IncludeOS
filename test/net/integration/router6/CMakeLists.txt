cmake_minimum_required(VERSION 3.0)

#service
project (service)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

include(os)

os_add_executable(net_router6 "Routing test ipv6" service.cpp)

os_add_drivers(net_router6 virtionet)
os_add_stdout(net_router6 default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(setup.sh ${CMAKE_CURRENT_BINARY_DIR})

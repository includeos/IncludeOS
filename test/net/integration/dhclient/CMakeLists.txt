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

os_add_executable(net_dhclient "IncludeOS DHCP test"  service.cpp)

os_add_plugins(net_dhclient autoconf)
os_add_drivers(net_dhclient virtionet)
os_add_stdout(net_dhclient default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

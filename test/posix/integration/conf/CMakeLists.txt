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
  test_sysconf.c
  test_pathconf.c
  test_pwd.c
)

#os_add_config(service "${CMAKE_CURRENT_SOURCE_DIR}/config.json")

os_add_executable(posix_conf "POSIX pathconf test" ${SOURCES})

os_add_plugins(posix_conf vfs)
os_add_stdout(posix_conf default_stdout)

# Create memdisk from folder
os_diskbuilder(posix_conf disk)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

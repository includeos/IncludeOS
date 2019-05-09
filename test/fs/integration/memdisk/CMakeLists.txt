cmake_minimum_required(VERSION 3.0)

project(memdisk)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

include(os)

set(SOURCES
    twosector.cpp
)

os_add_executable(fs_memdisk "Memdisk filesystem test" ${SOURCES})
os_add_stdout(fs_memdisk default_stdout)

os_add_memdisk(fs_memdisk ${CMAKE_CURRENT_SOURCE_DIR}/sector2.disk)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

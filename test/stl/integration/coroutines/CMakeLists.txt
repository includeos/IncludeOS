cmake_minimum_required(VERSION 3.0)

#service
project(service)
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

os_add_executable(stl_coroutines "C++ coroutines test" ${SOURCES})
os_add_stdout(stl_coroutines default_stdout)

os_compile_options(stl_coroutines PRIVATE "-fcoroutines-ts")
configure_file (test.py ${CMAKE_CURRENT_BINARY_DIR})

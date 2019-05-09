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

os_add_executable(posix_pthread "POSIX pthread test" ${SOURCES})

os_add_plugins(posix_pthread vfs)
os_add_stdout(posix_pthread default_stdout)

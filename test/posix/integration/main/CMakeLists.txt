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


option(NORMAL "" ON)
if (NORMAL)
  set(SOURCES service.cpp)
else()
  set(SOURCES main_no_params.cpp)
endif()

os_add_executable(posix_main "POSIX main test" ${SOURCES})

os_add_plugins(posix_main vfs)
os_add_stdout(posix_main default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

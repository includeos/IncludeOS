cmake_minimum_required(VERSION 3.0)

# Service
project (term)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

include(os)

set(SOURCES
    service.cpp # ...add more here
)
os_add_executable(kernel_timers "Timers Test Service" service.cpp timers.cpp)
os_add_stdout(kernel_timers default_stdout)
configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

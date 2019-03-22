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
  test_file_fd.cpp
)

#os_add_config(service "${CMAKE_CURRENT_SOURCE_DIR}/config.json")

os_add_executable(posix_file_fd "POSIX file descriptor test" ${SOURCES})

os_add_plugins(posix_file_fd vfs)
os_add_stdout(posix_file_fd default_stdout)

# Create memdisk from folder
os_diskbuilder(posix_file_fd disk)

configure_file (test.py ${CMAKE_CURRENT_BINARY_DIR})

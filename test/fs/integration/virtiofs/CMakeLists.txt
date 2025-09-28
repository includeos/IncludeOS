cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
  service.cpp
  )

os_add_executable(fs_virtiofs "VirtioFS integration test" ${SOURCES})
os_add_drivers(fs_virtiofs virtiofs)
os_add_stdout(fs_virtiofs default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(shared_dir.sh ${CMAKE_CURRENT_BINARY_DIR})
configure_file(banana.txt ${CMAKE_CURRENT_BINARY_DIR})

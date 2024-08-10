cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(service)
include(os)

set(SOURCES
    test_stat_ftw.cpp
    ftw_tests.cpp
    stat_tests.cpp
  )

os_add_executable(posix_stat "POSIX file descriptor test" ${SOURCES})

os_add_drivers(posix_stat boot_logger)
os_add_plugins(posix_stat vfs)
os_add_stdout(posix_stat default_stdout)

# Create memdisk from folder
os_diskbuilder(posix_stat disk)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

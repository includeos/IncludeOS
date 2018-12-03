cmake_minimum_required(VERSION 2.8.9)

project(chainloader)

#TODO cleanup ARCH in os.cmake and just add set platform?
set(ARCH i686)
set(PLATFORM x86_nano)

#TODO decide
#A includeos install could install os.cmake to a known location
#B alternative is to provide it via conan..
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

#yea baby
include(os)

option(default_stdout "" OFF)
set(SOURCES
    service.cpp hotswap.cpp
  )

os_add_executable(chainloader "IncludeOS chainloader" ${SOURCES})
os_install(TARGETS chainloader DESTINATION bin)

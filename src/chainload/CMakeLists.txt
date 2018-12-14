cmake_minimum_required(VERSION 2.8.9)

if (NOT DEFINED INCLUDEOS_PREFIX)
  if  (NOT DEFINED $ENV{INCLUDEOS_PREFIX})
    message(info "Setting default includeos path")
    set(INCLUDEOS_PREFIX /usr/local/includeos)
  else()
    set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
  endif()
endif()

project(chainloader)


#TODO cleanup ARCH in os.cmake and just add set platform?
set(ARCH i686)
set(PLATFORM x86_nano)

#TODO decide
#A includeos install could install os.cmake to a known location
#B alternative is to provide it via conan..
list(APPEND CMAKE_MODULE_PATH ${INCLUDEOS_PREFIX}/cmake)

#yea baby
include(os)

option(default_stdout "" OFF)
set(SOURCES
    service.cpp hotswap.cpp
  )

os_add_executable(chainloader "IncludeOS chainloader" ${SOURCES})
os_install(TARGETS chainloader DESTINATION bin)

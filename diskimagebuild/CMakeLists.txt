cmake_minimum_required(VERSION 2.8.9)

project (diskimagebuilder)

if (EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
else()
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
  endif()
  if (CONAN_PROFILE)
    set(CONANPROFILE PROFILE ${CONAN_PROFILE})
  endif()
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
     message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
     file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.13/conan.cmake"
                   "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()
  include(${CMAKE_BINARY_DIR}/conan.cmake)
  conan_cmake_run(
    CONANFILE conanfile.py
    BASIC_SETUP
    ${CONANPROFILE}
  )
endif()


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES main.cpp filetree.cpp writer.cpp)

add_executable(diskbuilder ${SOURCES})
target_link_libraries(diskbuilder stdc++)
#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

install(TARGETS diskbuilder DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

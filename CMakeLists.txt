cmake_minimum_required(VERSION 2.8.9)

project(mender_cli)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(BOTAN_VERSION 2)

# Custom includes
include_directories($ENV{INCLUDEOS_PREFIX}/include/botan-${BOTAN_VERSION})

include_directories($ENV{INCLUDEOS_PREFIX}/includeos/include/libcxx)
include_directories($ENV{INCLUDEOS_PREFIX}/includeos/api/sys)
include_directories($ENV{INCLUDEOS_PREFIX}/includeos/include/newlib)
include_directories($ENV{INCLUDEOS_PREFIX}/includeos/api/posix)
include_directories($ENV{INCLUDEOS_PREFIX}/includeos/api)
include_directories($ENV{INCLUDEOS_PREFIX}/includeos/include)
include_directories($ENV{INCLUDEOS_PREFIX}/include)

set(MENDER_CLI_OBJ
  auth_manager.cpp
  client.cpp
  keystore.cpp
  state.cpp
  )

add_library(mender_cli STATIC ${MENDER_CLI_OBJ})

set(LIBRARIES ${LIBRARIES}
  "${CMAKE_CURRENT_BINARY_DIR}/libmender_cli.a"
  "${CMAKE_SOURCE_DIR}/libliveupdate.a"
  $ENV{INCLUDEOS_PREFIX}/lib/libbotan-${BOTAN_VERSION}.a
  PARENT_SCOPE
  )

set(LOCAL_INCLUDES ${LOCAL_INCLUDES}
  $ENV{INCLUDEOS_PREFIX}/include/botan-${BOTAN_VERSION}
  PARENT_SCOPE
  )



#add_dependencies(service mender_cli)

#install(TARGETS mender_cli DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})
#install(DIRECTORY include/mana DESTINATION includeos/include)

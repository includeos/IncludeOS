cmake_minimum_required(VERSION 3.0)

option(REAL_HW "Run on real hardware" OFF)
option(LIVEUPDATE "Enable liveupdate" OFF)

# IncludeOS install location
if (NOT DEFINED INCLUDEOS_PREFIX)
  if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
    set(INCLUDEOS_PREFIX /usr/local/includeos)
  else()
    set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
  endif()
endif()

if (NOT EXISTS "${INCLUDEOS_PREFIX}/cmake/os.cmake")
  MESSAGE(FATAL_ERROR "IncludeOS does not appear to be installed at ${INCLUDEOS_PREFIX}")
endif()

list(APPEND CMAKE_MODULE_PATH ${INCLUDEOS_PREFIX}/cmake)

#service
project (liuircd)
include(os)

if(TARGET ircd)
  message(STATUS "ircd is already defined")
else()
  add_subdirectory(../IRCd/ircd ircd)
endif()


set(DRIVERS
    vmxnet3
  )

set(STDOUT
  default_stdout
)

os_add_executable(LiveUpdate "IRC service"  service.cpp)
os_link_libraries(LiveUpdate ircd)
os_add_drivers(LiveUpdate ${DRIVERS})
os_add_stdout(LiveUpdate ${STDOUT})

add_custom_command(
  OUTPUT motd.h
  COMMAND xxd -i < ${CMAKE_CURRENT_SOURCE_DIR}/ircd.motd > motd.h
  DEPENDS ircd.motd
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
add_custom_target(motd DEPENDS motd.h)
add_dependencies(LiveUpdate motd)

if (LIVEUPDATE)
  os_add_os_library(LiveUpdate liveupdate)
endif()

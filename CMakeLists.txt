cmake_minimum_required(VERSION 2.8.9)
project(service)

# default IncludeOS location #FIXME#
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(SERVICE_NAME "Live Update")
set(BINARY       "LiveUpdate")
set(MAX_MEM 24)
set(SOURCES
    service.cpp
  )
set(LOCAL_INCLUDES ".")

set(LIBRARIES ${CMAKE_BINARY_DIR}/libliveupdate.a)

set(DRIVERS
    virtionet
    silent_start
    #heap_debugging
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)

# LiveUpdate static library
add_library(liveupdate STATIC
    storage.cpp update.cpp resume.cpp hotswap.cpp serialize_tcp.cpp
  )
target_link_libraries(service liveupdate)
install(TARGETS liveupdate DESTINATION lib)

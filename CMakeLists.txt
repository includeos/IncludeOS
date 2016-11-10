cmake_minimum_required(VERSION 2.8.9)
project(includeos_service)

# default IncludeOS location #FIXME#
set(INCLUDEOS_ROOT $ENV{HOME}/includeos)

set(SERVICE_NAME "Live Update")
set(BINARY       "LiveUpdate")
set(MAX_MEM 128)
string(RANDOM LENGTH 7 ALPHABET 0123456789 STACK_PROTECTOR_VALUE)
set(SOURCES
    service.cpp storage.cpp update.cpp resume.cpp 
    hotswap.cpp serialize_tcp.cpp
  )
set(LOCAL_INCLUDES ".")

include(${INCLUDEOS_ROOT}/share/includeos/service.cmake)

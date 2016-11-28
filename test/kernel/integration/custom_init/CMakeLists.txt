cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(SERVICE_NAME "Custom initialization test")
set(BINARY       "test_custom_init")
set(MAX_MEM 128)
set(SOURCES
    service.cpp custom_init1.cpp custom_init2.cpp custom_init3.cpp
  )
#set(LOCAL_INCLUDES ".")

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/service.cmake)

cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)

project (test_plugin_init)

set(SERVICE_NAME "Plugin initialization test")
set(BINARY       "test_plugin_init")
set(MAX_MEM 128)
set(SOURCES
    service.cpp plugin1.cpp plugin2.cpp plugin3.cpp
  )
#set(LOCAL_INCLUDES ".")

set(PLUGINS example)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
project (service C CXX)

set(SERVICE_NAME "Linux userspace IP-stack fuzzer")
set(BINARY       "linux_fuzz")

set(SOURCES
  service.cpp fuzzy_packet.cpp fuzzy_stack.cpp fuzzy_webserver.cpp http.cpp
  )

#option(LIBCPP "" ON)
option(LIBFUZZER "" ON)
option(SANITIZE "" ON)
option(ENABLE_LTO "" OFF) # disable LTO because so much issues with LLD
option(STATIC_BUILD "" OFF) # asan doesnt support static builds
option(STRIP_BINARY "" OFF) # we need to see symbol names

include($ENV{INCLUDEOS_PREFIX}/cmake/linux.service.cmake)

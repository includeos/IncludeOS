cmake_minimum_required(VERSION 2.8.9)
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
project (service C CXX)

set(SERVICE_NAME "Linux userspace IP-stack fuzzer")
set(BINARY       "linux_fuzz")

set(SOURCES
  service.cpp fuzzy_packet.cpp fuzzy_stack.cpp
  )

option(LIBCPP "" ON)
option(LIBFUZZER "" ON)
option(SANITIZE "" ON)
option(STATIC_BUILD "" OFF) # asan doesnt support static builds

include($ENV{INCLUDEOS_PREFIX}/includeos/linux.service.cmake)

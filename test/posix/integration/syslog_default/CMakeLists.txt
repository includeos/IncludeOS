cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_ROOT})
  set(INCLUDEOS_ROOT /usr/local)
endif()

# Human-readable name of your service
set(SERVICE_NAME "Syslog Default Test Service")

# Name of your service binary
set(BINARY       "test_syslog_default")

# Maximum memory can be hard-coded into the binary
set(MAX_MEM 128)


# Source files to be linked with OS library parts to form bootable image
set(SOURCES
    service.cpp
    )

option(libvirtionet "" ON)

# To add your own include paths:
#set(LOCAL_INCLUDES ".")

# include service build script
include(${INCLUDEOS_ROOT}/includeos/service.cmake)

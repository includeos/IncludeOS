#
# Build and install plugins as libraries
#

# make LiveUpdate visible to plugins
include_directories(${INCLUDEOS_ROOT}/lib/LiveUpdate)

add_library(system_log STATIC "system_log.cpp")

add_library(syslogd STATIC syslogd.cpp)

add_library(unik STATIC unik.cpp)

add_library(example STATIC example.cpp)

add_library(autoconf STATIC autoconf.cpp)

add_library(terminal STATIC terminal.cpp)

add_library(terminal_liu STATIC terminal.cpp)
set_target_properties(terminal_liu PROPERTIES COMPILE_FLAGS "-DUSE_LIVEUPDATE")

add_library(nacl STATIC nacl.cpp)

add_library(vfs STATIC vfs.cpp)

add_library(field_medic STATIC
            field_medic/fieldmedic.cpp
            field_medic/diag.cpp)

add_library(madness STATIC
            madness/madness.cpp)

add_library(syslog STATIC syslog.cpp)

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

install(TARGETS
    system_log
    syslogd
    unik
    example
    autoconf
    terminal
    terminal_liu
    nacl
    vfs
    field_medic
    syslog
    madness
  DESTINATION ${ARCH}/plugins)

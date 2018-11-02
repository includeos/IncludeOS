#
# Build and install plugins as libraries
#

# make LiveUpdate visible to plugins
include_directories(${INCLUDEOS_ROOT}/lib/LiveUpdate)

add_library(system_log STATIC "system_log.cpp")
add_dependencies(system_log PrecompiledLibraries)

add_library(syslogd STATIC syslogd.cpp)
add_dependencies(syslogd PrecompiledLibraries)

add_library(unik STATIC unik.cpp)
add_dependencies(unik PrecompiledLibraries)

add_library(example STATIC example.cpp)
add_dependencies(example PrecompiledLibraries)

add_library(autoconf STATIC autoconf.cpp)
add_dependencies(autoconf PrecompiledLibraries)

add_library(terminal STATIC terminal.cpp)
add_dependencies(terminal PrecompiledLibraries)

add_library(terminal_liu STATIC terminal.cpp)
set_target_properties(terminal_liu PROPERTIES COMPILE_FLAGS "-DUSE_LIVEUPDATE")
add_dependencies(terminal_liu PrecompiledLibraries)

add_library(nacl STATIC nacl.cpp)
add_dependencies(nacl PrecompiledLibraries)

add_library(vfs STATIC vfs.cpp)
add_dependencies(vfs PrecompiledLibraries)

add_library(field_medic STATIC
            field_medic/fieldmedic.cpp
            field_medic/diag.cpp)
add_dependencies(field_medic PrecompiledLibraries)

add_library(madness STATIC
            madness/madness.cpp)
add_dependencies(madness PrecompiledLibraries)

add_library(syslog STATIC syslog.cpp)
add_dependencies(syslog PrecompiledLibraries)

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
  DESTINATION includeos/${ARCH}/plugins)

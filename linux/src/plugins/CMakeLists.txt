#
# Build and install plugins as libraries
#
set (IOS $ENV{INCLUDEOS_SRC})
set (PDIR "${IOS}/src/plugins")

# make LiveUpdate visible to plugins
include_directories(${IOS}/lib/LiveUpdate)
include_directories(${IOS}/mod/rapidjson/include)

#add_library(system_log STATIC "${PDIR}/system_log.cpp")
add_library(syslogd STATIC "${PDIR}/syslogd.cpp")
add_library(unik STATIC "${PDIR}/unik.cpp")
#add_library(example STATIC "${PDIR}/example.cpp")
add_library(autoconf STATIC "${PDIR}/autoconf.cpp")
add_library(terminal STATIC "${PDIR}/terminal.cpp")
add_library(terminal_liu STATIC "${PDIR}/terminal.cpp")
set_target_properties(terminal_liu PROPERTIES COMPILE_FLAGS "-DUSE_LIVEUPDATE")
add_library(nacl STATIC "${PDIR}/nacl.cpp")
add_library(vfs STATIC "${PDIR}/vfs.cpp")
add_library(field_medic STATIC
            "${PDIR}/field_medic/fieldmedic.cpp"
            "${PDIR}/field_medic/diag.cpp")
add_library(syslog STATIC "${PDIR}/syslog.cpp")

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

install(TARGETS
    #system_log
    syslogd
    unik
    #example
    autoconf
    terminal
    terminal_liu
    nacl
    vfs
    field_medic
    syslog
  DESTINATION includeos/linux/plugins)

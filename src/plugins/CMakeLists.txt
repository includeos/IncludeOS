# Add drivers as libraries
#
# Note: we can't have all the drivers in the same library as we'll want
# e.g. several network drivers, which will result in conflicting symbols
#
# ...There are probably nicer solutions, so please PR if you know them.
add_library(syslogd   STATIC syslogd.cpp)
add_library(unik      STATIC unik.cpp)

install(TARGETS syslogd unik DESTINATION includeos/plugins)

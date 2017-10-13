# Add drivers as libraries
#
# Note: we can't have all the drivers in the same library as we'll want
# e.g. several network drivers, which will result in conflicting symbols
#
# ...There are probably nicer solutions, so please PR if you know them.
add_library(syslogd STATIC syslogd.cpp)
add_dependencies(syslogd PrecompiledLibraries)

add_library(unik STATIC unik.cpp)
add_dependencies(unik PrecompiledLibraries)

add_library(example STATIC example.cpp)
add_dependencies(example PrecompiledLibraries)

add_library(autoconf STATIC autoconf.cpp)
add_dependencies(autoconf PrecompiledLibraries)

install(TARGETS syslogd unik example autoconf DESTINATION includeos/${ARCH}/plugins)

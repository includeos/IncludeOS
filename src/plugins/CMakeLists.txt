#
# Build and install plugins as libraries
#
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

add_library(nacl STATIC nacl.cpp)
add_dependencies(nacl PrecompiledLibraries)

install(TARGETS syslogd unik example autoconf terminal nacl DESTINATION includeos/${ARCH}/plugins)

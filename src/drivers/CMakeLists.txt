# Add drivers as libraries
#
# Note: we can't have all the drivers in the same library as we'll want
# e.g. several network drivers, which will result in conflicting symbols
#
# ...There are probably nicer solutions, so please PR if you know them.
add_library(virtioblk STATIC virtioblk.cpp)
add_library(virtionet STATIC virtionet.cpp)
add_library(heap_debugging STATIC heap_debugging.cpp)

install(TARGETS virtionet virtioblk heap_debugging DESTINATION includeos/drivers)

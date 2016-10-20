set(DRIVERS	virtioblk.cpp virtionet.cpp)

# add a library, this should build all the object files so that those can be included later on
add_library(drivers ${DRIVERS})

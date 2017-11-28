cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (example)

# Human-readable name of your service
set(SERVICE_NAME "SQlite3 Example Service")
# Name of your service binary
set(BINARY       "sqlite3_example")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
  service.cpp sqlite3_amalgamation/sqlite3.c
  )

set(LOCAL_INCLUDES "sqlite3_amalgamation/" "libsl3/include" "include")
add_definitions(-DSQLITE_THREADSAFE=0)
add_definitions(-DSQLITE_OMIT_LOAD_EXTENSION)
add_definitions(-DSQLITE_OMIT_WAL)

set(DRIVERS
  # ... Others from IncludeOS/src/drivers
  )

set(PLUGINS
  # ...others
  )


# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

set(SL3_SOURCES
    libsl3/src/sl3/columns.cpp
    libsl3/src/sl3/config.cpp
    libsl3/src/sl3/command.cpp
    libsl3/src/sl3/database.cpp
    libsl3/src/sl3/dataset.cpp
    libsl3/src/sl3/dbvalue.cpp
    libsl3/src/sl3/dbvalues.cpp
    libsl3/src/sl3/error.cpp
    libsl3/src/sl3/rowcallback.cpp
    libsl3/src/sl3/types.cpp
    libsl3/src/sl3/value.cpp
)
add_library(sl3 ${SL3_SOURCES})
target_link_libraries(service sl3)

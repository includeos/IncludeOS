cmake_minimum_required(VERSION 3.0)
# IncludeOS install location
if (NOT DEFINED INCLUDEOS_PREFIX)
  if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
    set(INCLUDEOS_PREFIX /usr/local/includeos)
  else()
    set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
  endif()
endif()

if (NOT EXISTS "${INCLUDEOS_PREFIX}/cmake/os.cmake")
  MESSAGE(FATAL_ERROR "IncludeOS does not appear to be installed at ${INCLUDEOS_PREFIX}")
endif()
list(APPEND CMAKE_MODULE_PATH ${INCLUDEOS_PREFIX}/cmake)

#service
project (service)
include(os)

set(SOURCES
  service.cpp sqlite3_amalgamation/sqlite3.c
)

os_add_executable(service "SQlite Service" ${SOURCES})

#os_add_drivers(demo ...)
#os_add_plugins( ... )
os_add_stdout(service default_stdout)

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
target_include_directories(sl3 PUBLIC
    "sqlite3_amalgamation/"
    "libsl3/include"
    "include"
  )
target_compile_definitions(sl3 PUBLIC SQLITE_THREADSAFE=0)
target_compile_definitions(sl3 PUBLIC SQLITE_OMIT_LOAD_EXTENSION)
target_compile_definitions(sl3 PUBLIC SQLITE_OMIT_WAL)

os_link_libraries(service sl3)

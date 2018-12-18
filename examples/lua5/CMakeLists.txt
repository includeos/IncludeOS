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
    service.cpp # ...add more here
)

os_add_executable(service "Lua Example Service" ${SOURCES})

add_library(lua53 STATIC IMPORTED)
set_target_properties(lua53 PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(lua53 PROPERTIES IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/liblua5.3.a")
os_link_libraries(service lua53)
os_include_directories(service PRIVATE "/usr/include/lua5.3")

#os_add_drivers(service boot_logger)
os_add_stdout(service default_stdout)

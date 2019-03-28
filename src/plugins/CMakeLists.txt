#
# Build and install plugins as libraries
#

# make LiveUpdate visible to plugins
include_directories(${CMAKE_SOURCE_DIR}/lib/LiveUpdate/include)
set(PLUGIN_SRCS
  system_log.cpp
  syslogd.cpp
  unik.cpp
  example.cpp
  autoconf.cpp
  nacl.cpp
  madness/madness.cpp
)

if (NOT ${PLATFORM} STREQUAL "nano")
  list(APPEND PLUGIN_SRCS
    vfs.cpp
    terminal.cpp
    syslog.cpp

  )
  #handle the more complex ones
  add_library(terminal_liu STATIC terminal.cpp)
  set_target_properties(terminal_liu PROPERTIES COMPILE_FLAGS "-DUSE_LIVEUPDATE")
  
endif()

set(PLUGINS "")
foreach(PLUGIN ${PLUGIN_SRCS})
  get_filename_component(PLUGIN_NAME ${PLUGIN} NAME_WE)
  add_library(${PLUGIN_NAME} STATIC ${PLUGIN})
  list(APPEND PLUGINS ${PLUGIN_NAME})
  install(TARGETS ${DRIVER_NAME} DESTINATION drivers)
endforeach()

add_library(field_medic STATIC
            field_medic/fieldmedic.cpp
            field_medic/diag.cpp)
list(APPEND PLUGINS field_medic)

#handle build targets for conan editable
set_target_properties(${PLUGINS}
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/plugins
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/plugins
)

#handle install of all plugins
install(TARGETS ${PLUGINS} DESTINATION plugins)

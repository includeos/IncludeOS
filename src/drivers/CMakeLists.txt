# Add drivers as libraries
#
# Note: we can't have all the drivers in the same library as we'll want
# e.g. several network drivers, which will result in conflicting symbols
#
# ...There are probably nicer solutions, so please PR if you know them.

# make LiveUpdate visible to drivers systemlog.. yet another DEP!!
include_directories(${INCLUDEOS_ROOT}/lib/LiveUpdate)

#TODO delete (including sources)
#add_library(ide_readwrite STATIC ide.cpp)
#set_target_properties(ide_readwrite PROPERTIES COMPILE_FLAGS "-DIDE_ENABLE_READ -DIDE_ENABLE_WRITE")

#add_library(ide_readonly STATIC ide.cpp)
#set_target_properties(ide_readonly PROPERTIES COMPILE_FLAGS "-DIDE_ENABLE_READ")

#add_library(ide_writeonly STATIC ide.cpp)
#set_target_properties(ide_writeonly PROPERTIES COMPILE_FLAGS "-DIDE_ENABLE_WRITE")

#delete heap_debugging.cpp
#END TODO

set(DRIVER_SRCS
  ip4_reassembly.cpp
  stdout/timestamps.cpp
)

if (${PLATFORM} STREQUAL "default")
  list(APPEND DRIVER_SRCS
      vga_emergency.cpp
    virtiocon.cpp
    virtioblk.cpp
    virtionet.cpp
    vmxnet3.cpp
    e1000.cpp
  )
elseif (${PLATFORM} STREQUAL "solo5-hvt" OR ${PLATFORM} STREQUAL "solo5-spt")
  list(APPEND DRIVER_SRCS
    solo5blk.cpp
    solo5net.cpp
  )
endif()
set(DRIVERS "")
foreach(DRIVER ${DRIVER_SRCS})
  get_filename_component(DRIVER_NAME ${DRIVER} NAME_WE)
  add_library(${DRIVER_NAME} STATIC ${DRIVER})
  list(APPEND DRIVERS ${DRIVER_NAME})
endforeach()

#TODO rename source file
add_library(boot_logger STATIC stdout/bootlog.cpp)
list(APPEND DRIVERS
  boot_logger
)
set_target_properties(${DRIVERS}
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/drivers
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/drivers
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/drivers
)
install(TARGETS ${DRIVERS} DESTINATION drivers)
# Simple stuff
add_library(vga_output STATIC stdout/vgaout.cpp)
add_library(default_stdout STATIC stdout/default_stdout.cpp)
# TODO DELETE
#add_library(vga_emergency STATIC vga_emergency.cpp)

set_target_properties(vga_output default_stdout
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/drivers/stdout
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/drivers/stdout
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/drivers/stdout
)

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

# installation of stdout drivers
install(TARGETS
    default_stdout
    vga_output
  #  vga_emergency
  DESTINATION drivers/stdout)

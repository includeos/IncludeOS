# Add drivers as libraries
#
# Note: we can't have all the drivers in the same library as we'll want
# e.g. several network drivers, which will result in conflicting symbols
#
# ...There are probably nicer solutions, so please PR if you know them.

add_library(disable_paging STATIC disable_paging.cpp)
add_dependencies(disable_paging PrecompiledLibraries)

add_library(ide_readwrite STATIC ide.cpp)
set_target_properties(ide_readwrite PROPERTIES COMPILE_FLAGS "-DIDE_ENABLE_READ -DIDE_ENABLE_WRITE")
add_dependencies(ide_readwrite PrecompiledLibraries)
add_library(ide_readonly STATIC ide.cpp)
set_target_properties(ide_readonly PROPERTIES COMPILE_FLAGS "-DIDE_ENABLE_READ")
add_dependencies(ide_readonly PrecompiledLibraries)
add_library(ide_writeonly STATIC ide.cpp)
set_target_properties(ide_writeonly PROPERTIES COMPILE_FLAGS "-DIDE_ENABLE_WRITE")
add_dependencies(ide_writeonly PrecompiledLibraries)

add_library(virtioblk STATIC virtioblk.cpp)
add_dependencies(virtioblk PrecompiledLibraries)

add_library(virtionet STATIC virtionet.cpp)
add_dependencies(virtionet PrecompiledLibraries)

add_library(vmxnet3 STATIC vmxnet3.cpp)
add_dependencies(vmxnet3 PrecompiledLibraries)

add_library(e1000 STATIC e1000.cpp)
add_dependencies(e1000 PrecompiledLibraries)

add_library(ip4_reassembly STATIC "ip4_reassembly.cpp")
add_dependencies(ip4_reassembly PrecompiledLibraries)

add_library(heap_debugging STATIC heap_debugging.cpp)
add_dependencies(heap_debugging PrecompiledLibraries)

add_library(boot_logger STATIC bootlog.cpp)
add_dependencies(boot_logger PrecompiledLibraries)

add_library(disk_logger STATIC "disk_logger.cpp")
add_dependencies(disk_logger PrecompiledLibraries)

add_library(disklog_reader STATIC "disklog_reader.cpp")
add_dependencies(disklog_reader PrecompiledLibraries)

add_library(vga_output STATIC vgaout.cpp)
add_dependencies(vga_output PrecompiledLibraries)

add_library(vga_emergency STATIC vga_emergency.cpp)
add_dependencies(vga_emergency PrecompiledLibraries)

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

install(TARGETS
    disable_paging
    ide_readwrite ide_readonly ide_writeonly
    virtionet virtioblk
    vmxnet3 e1000
    ip4_reassembly
    heap_debugging
    boot_logger disk_logger disklog_reader
    vga_output vga_emergency
    DESTINATION includeos/${ARCH}/drivers)

if(WITH_SOLO5)
  add_library(solo5blk STATIC solo5blk.cpp)
  add_dependencies(solo5blk PrecompiledLibraries)

  add_library(solo5net STATIC solo5net.cpp)
  add_dependencies(solo5net PrecompiledLibraries)
  install(TARGETS solo5net solo5blk DESTINATION includeos/${ARCH}/drivers)
endif(WITH_SOLO5)

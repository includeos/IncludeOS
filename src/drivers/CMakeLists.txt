# Add drivers as libraries
#
# Note: we can't have all the drivers in the same library as we'll want
# e.g. several network drivers, which will result in conflicting symbols
#
# ...There are probably nicer solutions, so please PR if you know them.
add_library(virtioblk STATIC virtioblk.cpp)
add_dependencies(virtioblk PrecompiledLibraries)

add_library(virtionet STATIC virtionet.cpp)
add_dependencies(virtionet PrecompiledLibraries)

add_library(vmxnet3 STATIC vmxnet3.cpp)
add_dependencies(vmxnet3 PrecompiledLibraries)

add_library(heap_debugging STATIC heap_debugging.cpp)
add_dependencies(heap_debugging PrecompiledLibraries)

add_library(silent_start STATIC silent.cpp)
add_dependencies(silent_start PrecompiledLibraries)

add_library(vga_output STATIC vgaout.cpp)
add_dependencies(vga_output PrecompiledLibraries)

add_library(solo5blk STATIC solo5blk.cpp)
add_dependencies(solo5blk PrecompiledLibraries)


install(TARGETS
    virtionet virtioblk
    solo5blk
    vmxnet3
    heap_debugging
    silent_start vga_output
    DESTINATION includeos/${ARCH}/drivers)

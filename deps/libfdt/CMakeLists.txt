cmake_minimum_required(VERSION 3.0)

project(libfdt)

set(SRC_S
  dtc/libfdt/fdt.c
  dtc/libfdt/fdt_ro.c
  dtc/libfdt/fdt_wip.c
  dtc/libfdt/fdt_sw.c
  dtc/libfdt/fdt_rw.c
  dtc/libfdt/fdt_strerror.c
  dtc/libfdt/fdt_empty_tree.c
  dtc/libfdt/fdt_addresses.c
  dtc/libfdt/fdt_overlay.c
)

set(HEADERS
  dtc/libfdt/fdt.h
  dtc/libfdt/libfdt.h
  dtc/libfdt/libfdt_env.h
)

include_directories(dtc/libfdt)

add_library(fdt STATIC ${SRC_S})

INSTALL(TARGETS fdt DESTINATION "lib")

INSTALL(FILES ${HEADERS} DESTINATION "include")

cmake_minimum_required(VERSION 2.8.9)
# default IncludeOS location #FIXME#
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project(service)

set(SERVICE_NAME "Live Update")
set(BINARY       "LiveUpdate")
set(SOURCES
    service.cpp
  )
set(LOCAL_INCLUDES ".")

set(LIBRARIES ${CMAKE_BINARY_DIR}/libliveupdate.a)

set(DRIVERS
    virtionet
    #silent_start
    #heap_debugging
  )

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

add_custom_command(
  OUTPUT hotswap64.o
	COMMAND ${CMAKE_ASM_NASM_COMPILER} -f bin -o hotswap64.bin ${CMAKE_CURRENT_SOURCE_DIR}/hotswap64.asm
	COMMAND ${CMAKE_OBJCOPY} -I binary -O elf64-x86-64 hotswap64.bin hotswap64.o
	DEPENDS hotswap64.asm
)

# LiveUpdate static library
add_library(liveupdate STATIC
    storage.cpp update.cpp resume.cpp rollback.cpp hotswap.cpp
    serialize_tcp.cpp hotswap64.o
  )
target_link_libraries(service liveupdate)
install(TARGETS liveupdate DESTINATION lib)

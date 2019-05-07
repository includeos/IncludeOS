
set(LIB_NAME aarch64_${PLATFORM})

set(PLATFORM_OBJECTS


#  start.asm
  start_aarch64.asm
  stop.asm
)

SET(PLATFORM_SOURCES
  serial1.cpp
  platform.cpp
  kernel_start.cpp
  os.cpp
  gic.cpp
  exception_handling.cpp
  init_libc.cpp
  sanity_checks.cpp
)

#add the arch in include path..
include_directories(../../arch/aarch64)
#include_directories(../x86_pc)

enable_language(ASM)
  #project(assembler C ASM)
set_source_files_properties(${PLATFORM_OBJECTS} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
  #add_executable(hello foo.s bar.c)

#  set_source_files_properties(start.asm PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

add_library(${LIB_NAME} STATIC ${PLATFORM_OBJECTS} ${PLATFORM_SOURCES})
#set_source_files_properties(${PLATFORM_SOURCES} PROPERIES LINKER_LANGUAGE CXX)
set_target_properties(${LIB_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${LIB_NAME}
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/platform
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/platform
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/platform
)

install(TARGETS ${LIB_NAME} DESTINATION platform)

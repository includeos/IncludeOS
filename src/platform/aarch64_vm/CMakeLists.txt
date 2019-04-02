
set(PLATFORM_OBJECTS

  serial1.cpp
#  start.asm
  start_aarch64.asm
)

SET(PLATFORM_SOURCES
  platform.cpp
  kernel_start.cpp
  os.cpp
  gic.cpp
  exception_handling.cpp
)

#add the arch in include path..
include_directories(../../arch/aarch64)

enable_language(ASM)
  #project(assembler C ASM)
set_source_files_properties(start_aarch64.asm PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
  #add_executable(hello foo.s bar.c)

#  set_source_files_properties(start.asm PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

add_library(aarch64_vm STATIC ${PLATFORM_OBJECTS} ${PLATFORM_SOURCES})
#set_source_files_properties(${PLATFORM_SOURCES} PROPERIES LINKER_LANGUAGE CXX)
set_target_properties(aarch64_vm PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(aarch64_vm
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/platform
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/platform
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/platform
)

install(TARGETS aarch64_vm DESTINATION platform)

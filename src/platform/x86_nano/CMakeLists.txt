set(LIBNAME ${ARCH}_nano)
set(PLATFORM_OBJECTS
  ../x86_pc/start.asm
  ../x86_pc/serial1.cpp
  ../x86_pc/idt.cpp
  platform.cpp
  kernel_start.cpp
  )

add_library(${LIBNAME} STATIC ${PLATFORM_OBJECTS})
set_target_properties(${LIBNAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${LIBNAME}
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/platform
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/platform
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/platform
)


install(TARGETS ${LIBNAME} DESTINATION platform)

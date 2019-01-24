
set(PLATFORM_OBJECTS
  platform.cpp
  kernel_start.cpp
  serial1.cpp
#  start.asm
  start_aarch64.asm
)

enable_language(ASM)
  #project(assembler C ASM)
set_source_files_properties(start_aarch64.asm PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
  #add_executable(hello foo.s bar.c)

#  set_source_files_properties(start.asm PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

add_library(aarch64_vm STATIC ${PLATFORM_OBJECTS})
set_target_properties(aarch64_vm PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS aarch64_vm DESTINATION ${ARCH}/platform)

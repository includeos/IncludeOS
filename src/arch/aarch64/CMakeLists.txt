
#set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf32")

### i686 arch specific ###
set(ARCH_OBJECTS
#  gdt_asm.asm
#  profile_intr.asm
#  apic_asm.asm
  arch_start.asm
#  exceptions.asm
#  interrupts.asm
#  fiber.asm
)
set(ARCH_SOURCES
  paging.cpp
)
enable_language(ASM)
#project(assembler C ASM)
set_source_files_properties(${ARCH_OBJECTS} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
#add_executable(hello foo.s bar.c)

add_library(arch STATIC ${ARCH_SOURCES} ${ARCH_OBJECTS})

#set_target_properties(  arch PROPERTIES LINKER_LANGUAGE CXX)
#add_library(crti STATIC crti.asm)
#add_library(crtn STATIC crtn.asm)

#set_target_properties(crti crtn arch PROPERTIES LINKER_LANGUAGE CXX)
#install(TARGETS crti crtn arch DESTINATION ${ARCH}/lib)
install(TARGETS arch DESTINATION ${ARCH}/lib)
install(FILES linker.ld DESTINATION ${ARCH})

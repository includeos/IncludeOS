
#set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf32")

### aarch64 arch specific ###
set(ARCH_OBJECTS
#  gdt_asm.asm
#  profile_intr.asm
#  apic_asm.asm
  arch_start.asm
  exceptions.asm
#  interrupts.asm
#  fiber.asm
)
set(ARCH_SOURCES
  paging.cpp
  cpu.cpp
  timer.cpp
)
enable_language(ASM)

set_source_files_properties(${ARCH_OBJECTS} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

add_library(arch STATIC ${ARCH_SOURCES} ${ARCH_OBJECTS})

set_target_properties(arch PROPERTIES LINKER_LANGUAGE CXX)
configure_file(linker.ld ${CMAKE_BINARY_DIR})

install(TARGETS arch DESTINATION ${ARCH}/lib)
install(FILES linker.ld DESTINATION ${ARCH})

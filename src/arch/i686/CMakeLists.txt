set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf32")

### i686 arch specific ###
set(ARCH_OBJECTS
  gdt_asm.asm
  profile_intr.asm
  apic_asm.asm
  arch_start.asm
  exceptions.asm
  interrupts.asm
  fiber.asm
  paging.cpp
)

add_library(arch STATIC ${ARCH_OBJECTS})

set_target_properties(arch PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS arch DESTINATION lib)
install(FILES linker.ld DESTINATION .)
configure_file(linker.ld ${CMAKE_BINARY_DIR})

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

### x86_64 arch specific ###
set(ARCH_OBJECTS
  apic_asm.asm
  apic_longmode.asm
  arch_start.asm
  exceptions.asm
  interrupts.asm
  fiber_asm.asm
  __syscall_entry.asm
  syscall_entry.cpp
  ist.cpp
  paging.cpp
  init_paging.cpp
)

add_library(arch STATIC ${ARCH_OBJECTS})
set_target_properties(arch PROPERTIES LINKER_LANGUAGE CXX)
configure_file(linker.ld ${CMAKE_BINARY_DIR})
install(TARGETS arch DESTINATION lib)
install(FILES linker.ld DESTINATION .)

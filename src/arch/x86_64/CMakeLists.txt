set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

### x86_64 arch specific ###
set(ARCH_OBJECTS
  apic_asm.asm
  apic_longmode.asm
  arch_start.asm
  exceptions.asm
  interrupts.asm
  fiber_asm.asm
  ist.cpp
  paging.cpp
  init_paging.cpp
)

add_library(arch STATIC ${ARCH_OBJECTS})
add_library(crti STATIC crti.asm)
add_library(crtn STATIC crtn.asm)

add_dependencies(arch PrecompiledLibraries)
set_target_properties(crti crtn arch PROPERTIES LINKER_LANGUAGE CXX)

install(TARGETS crti crtn arch DESTINATION includeos/${ARCH}/lib)
install(FILES linker.ld DESTINATION includeos/${ARCH})

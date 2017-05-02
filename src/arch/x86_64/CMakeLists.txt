set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

set(ARCH_OBJECTS
  ### x86 arch ###
  ../x86/start.asm
  ../x86/arch.cpp
  ../x86/gdt.cpp
  ../x86/acpi.cpp
  ../x86/ioapic.cpp
  ../x86/apic.cpp
  ../x86/apic_timer.cpp
  ../x86/apic_revenant.cpp
  ../x86/smp.cpp
  ../x86/pit.cpp
  ../x86/cpu_freq_sampling.cpp
  ../x86/pic.cpp

  ### x86_64 arch ###
  profile_intr.asm
  apic_asm.asm
  arch_start.asm
  interrupts.asm
  fiber_asm.asm
)

add_library(arch STATIC ${ARCH_OBJECTS})
add_library(crti STATIC crti.asm)
add_library(crtn STATIC crtn.asm)

add_subdirectory(../x86/boot ../x86/boot/build)

add_dependencies(arch PrecompiledLibraries)
set_target_properties(crti crtn arch PROPERTIES LINKER_LANGUAGE CXX)

install(TARGETS crti crtn arch DESTINATION includeos/${ARCH}/lib)

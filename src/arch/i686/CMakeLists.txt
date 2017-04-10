set(ARCH_OBJECTS
  ### x86 arch ###
  ../x86/start.asm #arch/x86/interrupts.asm arch/x86/profile_intr.asm
  ../x86/arch.cpp
  ../x86/gdt.cpp
  ../x86/acpi.cpp
  ../x86/ioapic.cpp
  ../x86/apic.cpp
  ../x86/apic_timer.cpp
  ../x86/apic_revenant.cpp
  ../x86/smp.cpp #../x86/apic_asm.asm
  ../x86/pit.cpp
  ../x86/cpu_freq_sampling.cpp
  ../x86/pic.cpp

  ### i686 arch ###
  gdt_asm.asm
  profile_intr.asm
  apic_asm.asm
  arch_start.asm
  interrupts.asm
)

add_library(arch STATIC ${ARCH_OBJECTS})
add_library(crti STATIC crti.asm)
add_library(crtn STATIC crtn.asm)

set_target_properties(crti crtn arch PROPERTIES LINKER_LANGUAGE CXX)

add_dependencies(arch PrecompiledLibraries)

install(TARGETS crti crtn arch DESTINATION includeos/${ARCH}/lib)

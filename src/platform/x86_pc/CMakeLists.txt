### x86 PC specific ###
set(X86_PC_OBJECTS
  start.asm
  platform.cpp
  gdt.cpp
  acpi.cpp
  ioapic.cpp
  apic.cpp
  apic_timer.cpp
  apic_revenant.cpp
  smp.cpp
  pit.cpp
  cpu_freq_sampling.cpp
  pic.cpp
  )

add_custom_command(
	OUTPUT apic_boot.o
	COMMAND ${CMAKE_ASM_NASM_COMPILER} -f bin -o apic_boot.bin ${CMAKE_CURRENT_SOURCE_DIR}/apic_boot.asm
	COMMAND ${CMAKE_OBJCOPY} -I binary -O ${OBJCOPY_TARGET} -B i386 apic_boot.bin apic_boot.o
	DEPENDS apic_boot.asm
)

add_library(x86_pc STATIC ${X86_PC_OBJECTS} apic_boot.o)
add_dependencies(x86_pc PrecompiledLibraries)

add_subdirectory(boot)
set_target_properties(x86_pc PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS x86_pc DESTINATION includeos/${ARCH}/platform/)

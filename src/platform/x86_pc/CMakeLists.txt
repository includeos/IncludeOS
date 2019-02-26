### x86 PC specific ###
set(X86_PC_OBJECTS
  start.asm
  kernel_start.cpp
  platform.cpp
  os.cpp
  clocks.cpp
  cmos.cpp
  cmos_clock.cpp
  gdt.cpp
  acpi.cpp
  ioapic.cpp
  apic.cpp
  apic_timer.cpp
  apic_revenant.cpp
  smp.cpp
  pit.cpp
  cpu_freq_sampling.cpp
  serial1.cpp
  pic.cpp
  softreset.cpp
  sanity_checks.cpp
  smbios.cpp
  idt.cpp
  ### KVM features ###
  ../kvm/kvmclock.cpp
  ../kvm/pv_eoi.cpp
  )

add_custom_command(
	OUTPUT apic_boot.o
	COMMAND ${CMAKE_ASM_NASM_COMPILER} -f bin -o apic_boot.bin ${CMAKE_CURRENT_SOURCE_DIR}/apic_boot.asm
	COMMAND ${CMAKE_OBJCOPY} -I binary -O ${OBJCOPY_TARGET} -B i386 apic_boot.bin apic_boot.o
	DEPENDS apic_boot.asm
)

add_library(x86_pc STATIC ${X86_PC_OBJECTS} apic_boot.o)

# disable sanitizers on kernel_start and others

add_subdirectory(boot)
set_target_properties(x86_pc PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS x86_pc DESTINATION ${ARCH}/platform/)

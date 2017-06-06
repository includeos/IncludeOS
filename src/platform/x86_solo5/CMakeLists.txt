
set(PLATFORM_OBJECTS
  ../x86_pc/acpi.cpp
  ../x86_pc/apic.cpp
  ../x86_pc/apic_timer.cpp
  ../x86_pc/ioapic.cpp
  ../x86_pc/pit.cpp
  ../x86_pc/cpu_freq_sampling.cpp
  ../../hw/serial1.cpp
  start.asm
  platform.cpp
  kernel_start.cpp
  )

add_library(x86_solo5 STATIC ${PLATFORM_OBJECTS})
add_dependencies(x86_solo5 PrecompiledLibraries)
set_target_properties(x86_solo5 PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS x86_solo5 DESTINATION includeos/${ARCH}/platform)


set(PLATFORM_OBJECTS
  ../x86_pc/start.asm
  ../x86_pc/acpi.cpp
  ../x86_pc/apic.cpp
  ../x86_pc/apic_timer.cpp
  ../x86_pc/ioapic.cpp
  ../x86_pc/pit.cpp
  ../x86_pc/cpu_freq_sampling.cpp
  ../x86_pc/serial1.cpp
  ../x86_pc/os.cpp
  ../x86_pc/block.cpp
  ../x86_pc/softreset.cpp
  platform.cpp
  kernel_start.cpp
  )

add_library(x86_nano STATIC ${PLATFORM_OBJECTS})
add_dependencies(x86_nano PrecompiledLibraries)
set_target_properties(x86_nano PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS x86_nano DESTINATION includeos/${ARCH}/platform)


set(PLATFORM_OBJECTS
  ../x86_pc/start.asm
  ../x86_pc/serial1.cpp
  ../x86_pc/idt.cpp
  platform.cpp
  kernel_start.cpp
  )

add_library(x86_nano STATIC ${PLATFORM_OBJECTS})
set_target_properties(x86_nano PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS x86_nano DESTINATION ${ARCH}/platform)

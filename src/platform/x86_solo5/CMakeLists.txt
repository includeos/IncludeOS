
set(PLATFORM_OBJECTS
  os.cpp
  serial1.cpp
  start.asm
  platform.cpp
  kernel_start.cpp
  sanity_checks.cpp
  )

add_library(x86_solo5 STATIC ${PLATFORM_OBJECTS})
set_target_properties(x86_solo5 PROPERTIES LINKER_LANGUAGE CXX)

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

install(TARGETS x86_solo5 DESTINATION ${ARCH}/platform)

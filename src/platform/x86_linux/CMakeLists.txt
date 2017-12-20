#
# x86 userspace Linux platform
#
set(PLATFORM_OBJECTS
  os.cpp
  serial1.cpp
  platform.cpp
  kernel_start.cpp
  sanity_checks.cpp
  )

add_library(x86_linux STATIC ${PLATFORM_OBJECTS})
add_dependencies(x86_linux PrecompiledLibraries)
set_target_properties(x86_linux PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS x86_linux DESTINATION includeos/${ARCH}/platform)

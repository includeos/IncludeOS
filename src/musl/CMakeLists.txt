
set(MUSL_OBJECTS
  futex.cpp
)

add_library(musl_syscalls STATIC ${MUSL_OBJECTS})
add_dependencies(musl_syscalls PrecompiledLibraries)

install(TARGETS musl_syscalls DESTINATION includeos/${ARCH}/lib)

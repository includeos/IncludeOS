cmake_minimum_required(VERSION 3.0)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)


project(hello_includeos)

# IncludeOS binary install location
# INCLUDEOS_SOURCE 👈 needed for rapid iteration on IncludeOS' cmake files

message(status "Libunwind at: ${INCLUDEOS_LIBUNWIND_PATH}")

set(LIBRARIES
  ${INCLUDEOS_PACKAGE}/lib/libos.a
  ${INCLUDEOS_PACKAGE}/platform/libx86_64_pc.a
  ${INCLUDEOS_PACKAGE}/lib/libarch.a
  ${INCLUDEOS_PACKAGE}/lib/libos.a
  # libosdeps
  ${INCLUDEOS_PACKAGE}/platform/libx86_64_pc.a
  ${INCLUDEOS_PACKAGE}/lib/libarch.a
  
  ${INCLUDEOS_PACKAGE}/lib/libmusl_syscalls.a
  ${INCLUDEOS_PACKAGE}/lib/libos.a
  
  ${INCLUDEOS_LIBCXX_PATH}
  ${INCLUDEOS_LIBCXXABI_PATH}
  ${INCLUDEOS_LIBUNWIND_PATH}
  
  ${INCLUDEOS_LIBC_PATH}
  ${INCLUDEOS_PACKAGE}/lib/libmusl_syscalls.a
  ${INCLDUEOS_LIBCXX_PATH}
  ${INCLUDEOS_LIBCXXABI_PATH}
  ${INCLUDEOS_PACKAGE}/lib/libos.a
  ${INCLUDEOS_LIBUNWIND_PATH}
  ${INCLUDEOS_LIBC_PATH}
)

include("${INCLUDEOS_PACKAGE}/cmake/os.cmake")

os_add_executable(hello_includeos "Hello world - OS included" src/main.cpp)
#os_add_drivers(hello virtionet vmxnet3 boot_logger)
os_add_stdout(hello_includeos default_stdout)

# This would be for application specific libs.
#os_link_libraries(hello_includeos PRIVATE your_custom_lib)


# TODO: remove or hide the ELF_POSTFIX.
# currently os.cmake expects the target name to be TARGET.elf.bin
# where TARGET is the first param to os_add_executable.
# I added it here becuase nix expects an install target.
set(BINARY_NAME hello_includeos)
set(ELF_TARGET ${BINARY_NAME}${ELF_POSTFIX})

install(TARGETS ${ELF_TARGET} DESTINATION bin)


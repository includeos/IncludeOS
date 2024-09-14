cmake_minimum_required(VERSION 3.6)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(hello_includeos)

find_package(IncludeOS)

include(os)

os_add_executable(hello_includeos "Hello world - OS included" src/main.cpp)
os_add_drivers(hello_includeos virtionet vmxnet3 boot_logger)
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

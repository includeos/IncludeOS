cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (service)
include(os)

set(SOURCES
    service.cpp
)

os_add_executable(kernel_grub "GRUB boot test" ${SOURCES})
os_add_stdout(kernel_grub default_stdout)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(grubiso.sh ${CMAKE_CURRENT_BINARY_DIR})
configure_file(grub.cfg ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command (OUTPUT grub.iso
  COMMAND bash ./grubiso.sh ${CMAKE_CURRENT_BINARY_DIR}/kernel_grub.elf.bin
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/)

add_custom_target(
  MakeGrubISO ALL
  DEPENDS grub.iso kernel_grub.elf.bin grubiso.sh)

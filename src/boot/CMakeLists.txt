add_library(multiboot OBJECT multiboot.cpp)

set_target_properties(multiboot PROPERTIES LINK_FLAGS "-nostdlib -melf_i386 -N --eh-frame-hdr --script=${INCLUDEOS_ROOT}/src/linker.ld --defsym=__stack_rand_ba=${STACK_PROTECTOR_VALUE}")

#add_executable(bootloader bootloader.asm disk_read_lba.asm)

add_custom_command(
	OUTPUT bootloader
	COMMAND nasm -f bin -I${CMAKE_CURRENT_SOURCE_DIR}/../ -o bootloader ${CMAKE_CURRENT_SOURCE_DIR}/bootloader.asm
	DEPENDS bootloader.asm disk_read_lba.asm
)

add_custom_target(run ALL DEPENDS bootloader)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bootloader DESTINATION share/includeos)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/multiboot.dir/multiboot.cpp.o DESTINATION share/includeos)

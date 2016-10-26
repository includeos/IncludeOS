set(SERVICE_NAME "\"\\\"IncludeOS Demo Service\\\"\"")
set(SERVICE "IncludeOS_256_color_vga")

enable_language(ASM_NASM)

# TODO: fix parameter pollution
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> -f elf32 -o <OBJECT> <SOURCE>")

set(SERVICE_SOURCES service.cpp int32.asm ${INCLUDEOS_INSTALL_DIR}/service_name.cpp)

add_executable(256_color_vga ${SERVICE_SOURCES} $<TARGET_OBJECTS:multiboot>)

set_target_properties(256_color_vga PROPERTIES COMPILE_FLAGS "${EXAMPLE_COMPILE_FLAGS} ${EXAMPLE_INCLUDES} -DSERVICE_NAME=${SERVICE_NAME} -DSERVICE=\\\"${SERVICE}\\\"")

set_target_properties(256_color_vga PROPERTIES LINK_FLAGS "${EXAMPLE_LINK_FLAGS}")

target_link_libraries(256_color_vga ${EXAMPLE_COMMON_LIBRARIES})

add_custom_command(
	OUTPUT ${SERVICE}.img
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/elf_syms 256_color_vga
	COMMAND objcopy --update-section .elf_symbols=_elf_symbols.bin 256_color_vga 256_color_vga
	COMMAND strip --strip-all --remove-section=.comment 256_color_vga
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/vmbuild ${CMAKE_BINARY_DIR}/examples/256_color_vga/256_color_vga ${CMAKE_BINARY_DIR}/src/boot/bootloader
	DEPENDS 256_color_vga
)

add_custom_target(${SERVICE} ALL DEPENDS ${SERVICE}.img)

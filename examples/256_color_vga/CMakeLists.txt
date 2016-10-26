set(SERVICE_NAME "\"\\\"IncludeOS Demo Service\\\"\"")
set(SERVICE "IncludeOS_256_color_vga")
set(SERVICE_ID "256_color_vga")

enable_language(ASM_NASM)

# TODO: fix parameter pollution
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> -f elf32 -o <OBJECT> <SOURCE>")

set(SERVICE_SOURCES int32.asm service.cpp ${INCLUDEOS_INSTALL_DIR}/service_name.cpp)

add_example(${SERVICE_ID} ${SERVICE} ${SERVICE_NAME} "${SERVICE_SOURCES}")

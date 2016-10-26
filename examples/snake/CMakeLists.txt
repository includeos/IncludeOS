set(SERVICE_NAME "\"\\\"Dual NIC Demo Service\\\"\"")
set(SERVICE "snake")

set(SERVICE_SOURCES service.cpp ${INCLUDEOS_INSTALL_DIR}/service_name.cpp)

add_executable(snake ${SERVICE_SOURCES} $<TARGET_OBJECTS:multiboot>)

set_target_properties(snake PROPERTIES COMPILE_FLAGS "${EXAMPLE_COMPILE_FLAGS} ${EXAMPLE_INCLUDES} -DSERVICE_NAME=${SERVICE_NAME} -DSERVICE=\\\"${SERVICE}\\\"")

set_target_properties(snake PROPERTIES LINK_FLAGS "${EXAMPLE_LINK_FLAGS}")

target_link_libraries(snake ${EXAMPLE_COMMON_LIBRARIES})

add_custom_command(
	OUTPUT ${SERVICE}.img
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/elf_syms snake
	COMMAND objcopy --update-section .elf_symbols=_elf_symbols.bin snake snake
	COMMAND strip --strip-all --remove-section=.comment snake
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/vmbuild ${CMAKE_BINARY_DIR}/examples/snake/snake ${CMAKE_BINARY_DIR}/src/boot/bootloader
	DEPENDS snake
)

add_custom_target(${SERVICE}_img ALL DEPENDS ${SERVICE}.img)

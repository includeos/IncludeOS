set(SERVICE_NAME "\"\\\"Dual NIC Demo Service\\\"\"")
set(SERVICE "tcp")

set(SERVICE_SOURCES service.cpp ${INCLUDEOS_INSTALL_DIR}/service_name.cpp)

add_executable(tcp ${SERVICE_SOURCES} $<TARGET_OBJECTS:multiboot>)

set_target_properties(tcp PROPERTIES COMPILE_FLAGS "${EXAMPLE_COMPILE_FLAGS} ${EXAMPLE_INCLUDES} -DSERVICE_NAME=${SERVICE_NAME}")

set_target_properties(tcp PROPERTIES LINK_FLAGS "${EXAMPLE_LINK_FLAGS}")

target_link_libraries(tcp ${EXAMPLE_COMMON_LIBRARIES})

add_custom_command(
	OUTPUT ${SERVICE}.img
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/elf_syms tcp
	COMMAND objcopy --update-section .elf_symbols=_elf_symbols.bin tcp tcp
	COMMAND strip --strip-all --remove-section=.comment tcp
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/vmbuild ${CMAKE_BINARY_DIR}/examples/tcp/tcp ${CMAKE_BINARY_DIR}/src/boot/bootloader
	DEPENDS tcp
)

add_custom_target(${SERVICE}_img ALL DEPENDS ${SERVICE}.img)

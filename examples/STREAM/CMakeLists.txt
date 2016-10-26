set(SERVICE_NAME "\"\\\"Dual NIC Demo Service\\\"\"")
set(SERVICE "STREAM")

set(SERVICE_SOURCES service.cpp stream.cpp ${INCLUDEOS_INSTALL_DIR}/service_name.cpp)

add_executable(STREAM ${SERVICE_SOURCES} $<TARGET_OBJECTS:multiboot>)

set_target_properties(STREAM PROPERTIES COMPILE_FLAGS "${EXAMPLE_COMPILE_FLAGS} ${EXAMPLE_INCLUDES} -DSERVICE_NAME=${SERVICE_NAME} -DSERVICE=\\\"${SERVICE}\\\"")

set_target_properties(STREAM PROPERTIES LINK_FLAGS "${EXAMPLE_LINK_FLAGS}")

target_link_libraries(STREAM ${EXAMPLE_COMMON_LIBRARIES})

add_custom_command(
	OUTPUT ${SERVICE}.img
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/elf_syms STREAM
	COMMAND objcopy --update-section .elf_symbols=_elf_symbols.bin STREAM STREAM
	COMMAND strip --strip-all --remove-section=.comment STREAM
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/vmbuild ${CMAKE_BINARY_DIR}/examples/STREAM/STREAM ${CMAKE_BINARY_DIR}/src/boot/bootloader
	DEPENDS STREAM
)

add_custom_target(${SERVICE}_img ALL DEPENDS ${SERVICE}.img)

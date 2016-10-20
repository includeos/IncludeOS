set(SERVICE_NAME "\"\\\"Dual NIC Demo Service\\\"\"")
set(SERVICE "smp_service")

set(SERVICE_SOURCES service.cpp ${INCLUDEOS_INSTALL_DIR}/service_name.cpp)

add_executable(smp_service ${SERVICE_SOURCES} $<TARGET_OBJECTS:multiboot>)

set_target_properties(smp_service PROPERTIES COMPILE_FLAGS "${EXAMPLE_COMPILE_FLAGS} ${EXAMPLE_INCLUDES} -DSERVICE_NAME=${SERVICE_NAME}")

set_target_properties(smp_service PROPERTIES LINK_FLAGS "${EXAMPLE_LINK_FLAGS}")

target_link_libraries(smp_service ${EXAMPLE_COMMON_LIBRARIES})

add_custom_command(
	OUTPUT ${SERVICE}.img
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/elf_syms smp_service
	COMMAND objcopy --update-section .elf_symbols=_elf_symbols.bin smp_service smp_service
	COMMAND strip --strip-all --remove-section=.comment smp_service
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/vmbuild ${CMAKE_BINARY_DIR}/examples/smp/smp_service ${CMAKE_BINARY_DIR}/src/boot/bootloader
	DEPENDS smp_service
)

add_custom_target(${SERVICE}_img ALL DEPENDS ${SERVICE}.img)

set(SERVICE_NAME "\"\\\"Dual NIC Demo Service\\\"\"")
set(SERVICE "dual_nic_service")

set(SERVICE_SOURCES service.cpp ${INCLUDEOS_INSTALL_DIR}/service_name.cpp)

add_executable(dual_nic_service ${SERVICE_SOURCES} $<TARGET_OBJECTS:multiboot>)

set_target_properties(dual_nic_service PROPERTIES COMPILE_FLAGS "${EXAMPLE_COMPILE_FLAGS} ${EXAMPLE_INCLUDES} -DSERVICE_NAME=${SERVICE_NAME} -DSERVICE=\\\"${SERVICE}\\\"")

set_target_properties(dual_nic_service PROPERTIES LINK_FLAGS "${EXAMPLE_LINK_FLAGS}")

target_link_libraries(dual_nic_service ${EXAMPLE_COMMON_LIBRARIES})

add_custom_command(
	OUTPUT ${SERVICE}.img
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/elf_syms dual_nic_service
	COMMAND objcopy --update-section .elf_symbols=_elf_symbols.bin dual_nic_service dual_nic_service
	COMMAND strip --strip-all --remove-section=.comment dual_nic_service
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/vmbuild ${CMAKE_BINARY_DIR}/examples/dualnic/dual_nic_service ${CMAKE_BINARY_DIR}/src/boot/bootloader
	DEPENDS dual_nic_service
)

add_custom_target(${SERVICE}_img ALL DEPENDS ${SERVICE}.img)

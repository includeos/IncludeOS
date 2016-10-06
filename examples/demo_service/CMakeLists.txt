set(SERVICE_NAME "\"\\\"IncludeOS Demo Service\\\"\"")
set(SERVICE "IncludeOS_Demo_Service")

set(SERVICE_SOURCES service.cpp ${INCLUDEOS_INSTALL_DIR}/service_name.cpp)

add_executable(test_service ${SERVICE_SOURCES} $<TARGET_OBJECTS:multiboot>)

set_target_properties(test_service PROPERTIES COMPILE_FLAGS "${EXAMPLE_COMPILE_FLAGS} ${EXAMPLE_INCLUDES} -DSERVICE_NAME=${SERVICE_NAME}")

set_target_properties(test_service PROPERTIES LINK_FLAGS "${EXAMPLE_LINK_FLAGS}")

target_link_libraries(test_service ${EXAMPLE_COMMON_LIBRARIES})

add_custom_command(
	OUTPUT ${SERVICE}.img
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/elf_syms test_service
	COMMAND objcopy --update-section .elf_symbols=_elf_symbols.bin test_service test_service
	COMMAND strip --strip-all --remove-section=.comment test_service
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/vmbuild ${CMAKE_BINARY_DIR}/examples/demo_service/test_service ${CMAKE_BINARY_DIR}/src/boot/bootloader
	DEPENDS test_service
)

add_custom_target(${SERVICE} ALL DEPENDS ${SERVICE}.img)

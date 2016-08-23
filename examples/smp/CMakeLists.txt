# this is a very ugly hack. I wanted to build a test service to have
# something to check basic functionality against.
#
# TODO: use direct library references, remove add_custom_command

set(SERVICE_NAME "\"\\\"SMP Example Service\\\"\"")
set(SERVICE "SMP")

add_custom_command(
	OUTPUT ${SERVICE}.img
	COMMAND /usr/bin/clang++ ${EXAMPLE_CLANG_OPTIONS} -o service.o ${CMAKE_CURRENT_SOURCE_DIR}/service.cpp
	COMMAND /usr/bin/clang++ ${EXAMPLE_CLANG_OPTIONS} -DSERVICE_NAME="${SERVICE_NAME}" -o .service_name.o ${INCLUDEOS_INSTALL_DIR}/service_name.cpp
	COMMAND ld ${EXAMPLE_LD_OPTIONS} service.o .service_name.o ${EXAMPLE_LD_LIBS} -o ${SERVICE} #${CMAKE_BINARY_DIR}/drivers/virtionet.o 
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/vmbuild ${SERVICE} ${CMAKE_BINARY_DIR}/src/boot/bootloader
	DEPENDS service.cpp
	OUTPUT ${SERVICE}.img
)

add_custom_target(${SERVICE} ALL DEPENDS ${SERVICE}.img)

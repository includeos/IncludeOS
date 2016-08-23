# this is a very ugly hack. I wanted to build a test service to have
# something to check basic functionality against.
#
# TODO: use direct library references, remove add_custom_command

set(SERVICE_NAME "\"\\\"STREAM Memory Benchmark\\\"\"")
set(SERVICE "STREAM")

add_custom_command(
	OUTPUT ${SERVICE}.img
	COMMAND /usr/bin/clang++ -target i686-elf -mstackrealign -msse3 -fstack-protector-strong -O2 -Wall -Wextra  -c -m32 -std=c++14 -I${INCLUDEOS_INSTALL_DIR}/libcxx/include -I${INCLUDEOS_INSTALL_DIR}/api/sys -I${INCLUDEOS_INSTALL_DIR}/newlib/include -I${INCLUDEOS_INSTALL_DIR}/api -I${INCLUDEOS_INSTALL_DIR}/mod/GSL/ -D_LIBCPP_HAS_NO_THREADS=1 -D_GNU_SOURCE -o service.o ${CMAKE_CURRENT_SOURCE_DIR}/stream.cpp
	COMMAND /usr/bin/clang++ -target i686-elf -mstackrealign -msse3 -fstack-protector-strong -O2 -Wall -Wextra  -c -m32 -std=c++14 -I${INCLUDEOS_INSTALL_DIR}/libcxx/include -I${INCLUDEOS_INSTALL_DIR}/api/sys -I${INCLUDEOS_INSTALL_DIR}/newlib/include -I${INCLUDEOS_INSTALL_DIR}/api -I${INCLUDEOS_INSTALL_DIR}/mod/GSL/ -D_LIBCPP_HAS_NO_THREADS=1 -D_GNU_SOURCE -DSERVICE_NAME="${SERVICE_NAME}" -o .service_name.o ${INCLUDEOS_INSTALL_DIR}/service_name.cpp
	COMMAND ld -nostdlib -melf_i386 -N --eh-frame-hdr --script=${INCLUDEOS_INSTALL_DIR}/linker.ld ${INCLUDEOS_INSTALL_DIR}/crt/crtbegin.o ${INCLUDEOS_INSTALL_DIR}/crt/crti.o ${INCLUDEOS_INSTALL_DIR}/multiboot.o service.o .service_name.o ${CMAKE_BINARY_DIR}/src/libos.a ${INCLUDEOS_INSTALL_DIR}/libcxx/libc++.a ${CMAKE_BINARY_DIR}/src/liblibc++abi.a.a ${CMAKE_BINARY_DIR}/src/libos.a ${INCLUDEOS_INSTALL_DIR}/newlib/libc.a ${INCLUDEOS_INSTALL_DIR}/newlib/libm.a ${INCLUDEOS_INSTALL_DIR}/libgcc/libgcc.a ${INCLUDEOS_INSTALL_DIR}/crt/crtend.o ${INCLUDEOS_INSTALL_DIR}/crt/crtn.o -o ${SERVICE}
	COMMAND ${CMAKE_BINARY_DIR}/vmbuild/vmbuild ${SERVICE} ${CMAKE_BINARY_DIR}/src/boot/bootloader
	DEPENDS service.cpp
)

add_custom_target(${SERVICE} ALL DEPENDS ${SERVICE}.img)

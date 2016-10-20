set(MAX_MEM 128)

SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS) # this removed -rdynamic from linker output
SET(CMAKE_CXX_LINK_EXECUTABLE "/usr/bin/ld <OBJECTS> -o  <TARGET> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")

set(BUILD_SHARED_LIBRARIES OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

set(EXAMPLE_COMPILE_FLAGS "-target i686-elf -mstackrealign -msse3 -fstack-protector-strong -O2 -Wall -Wextra  -c -m32 -std=c++14  -D_LIBCPP_HAS_NO_THREADS=1 -D_GNU_SOURCE")

set(EXAMPLE_LINK_FLAGS "-nostdlib -melf_i386 -N --eh-frame-hdr --script=${INCLUDEOS_INSTALL_DIR}/linker.ld --defsym=_MAX_MEM_MIB_=${MAX_MEM} --defsym=__stack_rand_ba=${STACK_PROTECTOR_VALUE} ${INCLUDEOS_INSTALL_DIR}/crt/crtbegin.o ${INCLUDEOS_INSTALL_DIR}/crt/crti.o")

set(EXAMPLE_INCLUDES "-I${INCLUDEOS_INSTALL_DIR}/libcxx/include -I${INCLUDEOS_INSTALL_DIR}/api/sys -I${INCLUDEOS_INSTALL_DIR}/newlib/include -I${INCLUDEOS_INSTALL_DIR}/api -I${INCLUDEOS_INSTALL_DIR}/mod/GSL/")

set(EXAMPLE_COMMON_LIBRARIES os ${INCLUDEOS_INSTALL_DIR}/libcxx/libc++.a libc++abi.a os ${INCLUDEOS_INSTALL_DIR}/newlib/libc.a ${INCLUDEOS_INSTALL_DIR}/newlib/libm.a ${INCLUDEOS_INSTALL_DIR}/libgcc/libgcc.a ${INCLUDEOS_INSTALL_DIR}/crt/crtend.o ${INCLUDEOS_INSTALL_DIR}/crt/crtn.o)

add_subdirectory(256_color_vga)
add_subdirectory(demo_service)
add_subdirectory(dualnic)
add_subdirectory(smp)
add_subdirectory(snake)
add_subdirectory(STREAM)
add_subdirectory(tcp)

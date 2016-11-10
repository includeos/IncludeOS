set(MAX_MEM 128)

SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS) # this removed -rdynamic from linker output
SET(CMAKE_CXX_LINK_EXECUTABLE "/usr/bin/ld <OBJECTS> -o  <TARGET> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")

set(BUILD_SHARED_LIBRARIES OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(SERVICE_STUB "${INCLUDEOS_ROOT}/src/seed/service_name.cpp")

set(EXAMPLE_COMPILE_FLAGS "-target i686-elf -mstackrealign -msse3 -fstack-protector-strong -O2 ${WARNS} -c -m32 -std=c++14 -D_LIBCPP_HAS_NO_THREADS=1 -D_GNU_SOURCE")

set(EXAMPLE_LINK_FLAGS "-nostdlib -melf_i386 -N --eh-frame-hdr --script=${INCLUDEOS_ROOT}/src/linker.ld --defsym=_MAX_MEM_MIB_=${MAX_MEM} --defsym=__stack_rand_ba=${STACK_PROTECTOR_VALUE} ${CRTBEGIN} ${CMAKE_BINARY_DIR}/src/crti.o")

set(EXAMPLE_INCLUDES ${LIBCXX_INCLUDE_DIR} ${INCLUDEOS_ROOT}/api/sys ${NEWLIB_INCLUDE_DIR} ${INCLUDEOS_ROOT}/api ${INCLUDEOS_ROOT}/mod/GSL/)

set(CRTN ${CMAKE_BINARY_DIR}/src/crtn.o)

set(EXAMPLE_COMMON_LIBRARIES os libcxx c++abi os libc libm libgcc ${CRTEND} ${CRTN})

function(add_example id example_name example_description example_sources)
	add_executable(${id} ${example_sources} $<TARGET_OBJECTS:multiboot>)

	set_target_properties(${id} PROPERTIES COMPILE_FLAGS ${EXAMPLE_COMPILE_FLAGS})

	set_target_properties(${id} PROPERTIES LINK_FLAGS "${EXAMPLE_LINK_FLAGS}")

	target_compile_definitions(${id} PUBLIC SERVICE="${example_name}" SERVICE_NAME=${example_description})
	target_include_directories(${id} PUBLIC ${EXAMPLE_INCLUDES})
	target_link_libraries(${id} ${EXAMPLE_COMMON_LIBRARIES})

	add_dependencies(${id} crti crtn libcxx libc libm)

	add_custom_command(
		OUTPUT ${example_name}.img
		COMMAND ${CMAKE_BINARY_DIR}/vmbuild/elf_syms ${id}
		COMMAND objcopy --update-section .elf_symbols=_elf_symbols.bin ${id} ${id}
		COMMAND strip --strip-all --remove-section=.comment ${id}
		COMMAND ${CMAKE_BINARY_DIR}/vmbuild/vmbuild ${CMAKE_BINARY_DIR}/examples/${id}/${id} ${CMAKE_BINARY_DIR}/src/boot/bootloader
		DEPENDS ${id}
	)

	add_custom_target(${example_name} ALL DEPENDS ${example_name}.img)
endfunction(add_example id example_name example_description example_sources)


add_subdirectory(256_color_vga)
add_subdirectory(demo_service)
add_subdirectory(dualnic)
add_subdirectory(scoped_profiler)
add_subdirectory(smp)
add_subdirectory(snake)
add_subdirectory(STREAM)
add_subdirectory(tcp)

cmake_minimum_required(VERSION 2.8.9)
project(includeos_service)

set(INCLUDEOS_ROOT $ENV{HOME}/includeos)

set(SERVICE_NAME "IRC server")
set(BINARY       "IRCd")
set(MAX_MEM 128)
set(SOURCES
    channel.cpp     client.cpp      client_new.cpp  modes.cpp    test.cpp     transform.cpp
    client_cmd.cpp  client_msg.cpp  ircd.cpp        service.cpp  timeout.cpp
  )
set(LOCAL_INCLUDES ".")

# test compiler
if(CMAKE_COMPILER_IS_GNUCC)
	# currently gcc is not supported due to problems cross-compiling a unikernel
	# (i.e., building a 32bit unikernel (only supported for now) on a 64bit system)
	message(FATAL_ERROR "GCC is not currently supported, please clean-up build directory and configure for clang through CC and CXX environment variables")
endif(CMAKE_COMPILER_IS_GNUCC)

set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf")
enable_language(ASM_NASM)

# stackrealign is needed to guarantee 16-byte stack alignment for SSE
# the compiler seems to be really dumb in this regard, creating a misaligned stack left and right
set(CAPABS "-mstackrealign -msse3 -fstack-protector-strong")

# Various global defines
# * OS_TERMINATE_ON_CONTRACT_VIOLATION provides classic assert-like output from Expects / Ensures
# * _GNU_SOURCE enables POSIX-extensions in newlib, such as strnlen. ("everything newlib has", ref. cdefs.h)
set(CAPABS "${CAPABS} -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE -DSERVICE=${BINARY}")
set(WARNS  "-Wall -Wextra") #-pedantic

# configure options
option(debug "Build with debugging symbols (OBS: Dramatically increases binary size)" OFF)
option(debug-info "Build like \"all\" but with debugging output (i.e. the 'debug'-macro) enabled" OFF)
option(debug-all "Build with debugging symbols + debugging output, i.e. \"debug\" + \"debug-info\"" OFF)
option(minimal "Build for minimal size" OFF)
option(stripped "reduce size" OFF)


set(OPTIMIZE "-O2")
# these kinda work with llvm
set(CMAKE_CXX_FLAGS "-MMD -target i686-elf ${CAPABS} ${OPTIMIZE} ${WARNS} -c -m32 -std=c++14 -D_LIBCPP_HAS_NO_THREADS=1")
set(CMAKE_C_FLAGS "-MMD -target i686-elf ${CAPABS} ${OPTIMIZE} ${WARNS} -c -m32")

# executable
set(SERVICE_STUB "${INCLUDEOS_ROOT}/share/includeos/service_name.cpp")

add_executable(service ${SOURCES} ${SERVICE_STUB})
set_target_properties(service PROPERTIES OUTPUT_NAME ${BINARY})

# includes
include_directories(${LOCAL_INCLUDES})
include_directories(${INCLUDEOS_ROOT}/include/libcxx)
include_directories(${INCLUDEOS_ROOT}/include/api/sys)
include_directories(${INCLUDEOS_ROOT}/include/newlib)
include_directories(${INCLUDEOS_ROOT}/include/api/posix)
include_directories(${INCLUDEOS_ROOT}/include/api)
include_directories(${INCLUDEOS_ROOT}/include/gsl)


# linker stuff
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS) # this removed -rdynamic from linker output
set(CMAKE_CXX_LINK_EXECUTABLE "/usr/bin/ld <OBJECTS> -o  <TARGET> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")

set(BUILD_SHARED_LIBRARIES OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

set(CRTBEG ${INCLUDEOS_ROOT}/share/crtbegin.o ${INCLUDEOS_ROOT}/share/crti.o ${INCLUDEOS_ROOT}/share/multiboot.o)
set(CRTEND ${INCLUDEOS_ROOT}/share/crtend.o ${INCLUDEOS_ROOT}/share/crtn.o)

set(LDFLAGS "-nostdlib -melf_i386 -N --eh-frame-hdr --script=${INCLUDEOS_ROOT}/share/includeos/linker.ld --defsym=_MAX_MEM_MIB_=${MAX_MEM} --defsym=__stack_rand_ba=${STACK_PROTECTOR_VALUE} ${CRTBEGIN})
set_target_properties(service PROPERTIES LINK_FLAGS "${LDFLAGS}")

set_target_properties(service PROPERTIES OUTPUT_NAME ${BINARY})

set(LIBRARIES 
    ${INCLUDEOS_ROOT}/lib/libos.a
    ${INCLUDEOS_ROOT}/lib/libc++.a
    ${INCLUDEOS_ROOT}/lib/libc++abi.a
    ${INCLUDEOS_ROOT}/lib/libc.a
    ${INCLUDEOS_ROOT}/lib/libm.a
    ${INCLUDEOS_ROOT}/lib/libg.a
    ${INCLUDEOS_ROOT}/lib/libgcc.a
    ${CRTEND})
target_link_libraries(service ${LIBRARIES})

add_custom_command(
  OUTPUT  ${BINARY}.img
  COMMAND ${INCLUDEOS_ROOT}/bin/elf_syms ${BINARY}
  COMMAND objcopy --update-section .elf_symbols=_elf_symbols.bin ${BINARY} ${BINARY}
  COMMAND ${INCLUDEOS_ROOT}/bin/vmbuild ${BINARY} ${INCLUDEOS_ROOT}/share/includeos/bootloader
  DEPENDS ${BINARY}
)

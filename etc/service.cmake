#
# 
#

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
set(CAPABS "${CAPABS} -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE -DSERVICE=\"\\\"${BINARY}\\\"\" -DSERVICE_NAME=\"\\\"${SERVICE_NAME}\\\"\"")
set(WARNS  "-Wall -Wextra") #-pedantic

# configure options
option(debug "Build with debugging symbols (OBS: increases binary size)" OFF)
option(minimal "Build for minimal size" OFF)
option(stripped "reduce size" OFF)

# DRIVERS
set(DRIVERS)

file(GLOB DRIVER_LIST "${INCLUDEOS_ROOT}/lib/includeos-drivers/*.a")
foreach(FILENAME ${DRIVER_LIST})
  get_filename_component(OPTNAME ${FILENAME} NAME_WE)
  option(${OPTNAME} "Add ${OPTNAME} driver" OFF)
  if (${OPTNAME})
      list(APPEND DRIVERS ${FILENAME})
  endif()
endforeach()

set(OPTIMIZE "-O2")
if (minimal)
  set(OPTIMIZE "-Os")
endif()

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
set(CMAKE_CXX_LINK_EXECUTABLE "/usr/bin/ld -o  <TARGET> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> <LINK_LIBRARIES>")

set(BUILD_SHARED_LIBRARIES OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

set(STRIP_LV)
if (NOT debug)
  set(STRIP_LV "--strip-debug")
endif()
if (stripped)
  set(STRIP_LV "--strip-all")
endif()

set(LDFLAGS "-nostdlib -melf_i386 -N --eh-frame-hdr ${STRIP_LV} --script=${INCLUDEOS_ROOT}/share/includeos/linker.ld --defsym=_MAX_MEM_MIB_=${MAX_MEM} --defsym=_STACK_GUARD_VALUE_=${STACK_PROTECTOR_VALUE} ${INCLUDEOS_ROOT}/share/includeos/crtbegin.o ${INCLUDEOS_ROOT}/share/includeos/crti.o ${INCLUDEOS_ROOT}/share/includeos/multiboot.cpp.o")
set_target_properties(service PROPERTIES LINK_FLAGS "${LDFLAGS}")

add_library(libos STATIC IMPORTED)
set_target_properties(libos PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libos PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_ROOT}/lib/libos.a)

add_library(libcxx STATIC IMPORTED)
add_library(cxxabi STATIC IMPORTED)
set_target_properties(libcxx PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libcxx PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_ROOT}/lib/libc++.a)
set_target_properties(cxxabi PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(cxxabi PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_ROOT}/lib/libc++abi.a)

add_library(libc STATIC IMPORTED)
set_target_properties(libc PROPERTIES LINKER_LANGUAGE C)
set_target_properties(libc PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_ROOT}/lib/libc.a)
add_library(libm STATIC IMPORTED)
set_target_properties(libm PROPERTIES LINKER_LANGUAGE C)
set_target_properties(libm PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_ROOT}/lib/libm.a)
add_library(libg STATIC IMPORTED)
set_target_properties(libg PROPERTIES LINKER_LANGUAGE C)
set_target_properties(libg PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_ROOT}/lib/libg.a)
add_library(libgcc STATIC IMPORTED)
set_target_properties(libgcc PROPERTIES LINKER_LANGUAGE C)
set_target_properties(libgcc PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_ROOT}/lib/libgcc.a)

# add drivers before the other libraries
foreach(DRIVER ${DRIVERS})
  get_filename_component(DNAME ${DRIVER} NAME_WE)

  add_library(${DNAME} STATIC IMPORTED)
  set_target_properties(${DNAME} PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(${DNAME} PROPERTIES IMPORTED_LOCATION ${DRIVER})

  target_link_libraries(service --whole-archive ${DNAME} --no-whole-archive)
endforeach()

# all the OS and C/C++ libraries + crt end
target_link_libraries(service
    libos
    libcxx
    cxxabi
    libos
    libc
    libm
    libg
    libgcc
    ${INCLUDEOS_ROOT}/share/includeos/crtend.o
    ${INCLUDEOS_ROOT}/share/includeos/crtn.o
  )

set(STRIP_LV strip --strip-all ${BINARY})
if (debug)
  set(STRIP_LV /bin/true)
endif()

add_custom_command(
  TARGET  service POST_BUILD
  COMMAND ${INCLUDEOS_ROOT}/bin/elf_syms ${BINARY}
  COMMAND objcopy --update-section .elf_symbols=_elf_symbols.bin ${BINARY} ${BINARY}
  COMMAND ${STRIP_LV}
  COMMAND rm _elf_symbols.bin
)

add_custom_command(
  OUTPUT  ${BINARY}.img
  COMMAND ${INCLUDEOS_ROOT}/bin/vmbuild ${BINARY} ${INCLUDEOS_ROOT}/share/includeos/bootloader
  DEPENDS ${BINARY}
)
add_custom_target(${BINARY}.img ALL DEPENDS ${BINARY})

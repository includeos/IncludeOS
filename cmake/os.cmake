option(CONFIG_JSON "Json configuration file default config.json" ON)



set(CPP_VERSION c++17)
set (CMAKE_CXX_STANDARD 17)

if (${CMAKE_VERSION} VERSION_LESS "3.12")
  find_program(Python2 python2.7)
  if (NOT Python2)
    #brutal fallback
    set(Python2_EXECUTABLE python)
  else()
    set(Python2_EXECUTABLE ${Python2})
  endif()
else()
  find_package(Python2 COMPONENTS Interpreter)
endif()

if (NOT DEFINED PLATFORM)
  if (DEFINED ENV{PLATFORM})
    set(PLATFORM $ENV{PLATFORM})
  else()
    set(PLATFORM x86_pc)
  endif()
endif()

#TODO move this into sub scripts conan.cmake and normal.cmake
if(CONAN_EXPORTED)
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()

  #TODO use these
  #CONAN_SETTINGS_ARCH Provides arch type
  #CONAN_SETTINGS_BUILD_TYPE provides std cmake "Debug" and "Release" "are they set by conan_basic?"
  #CONAN_SETTINGS_COMPILER AND CONAN_SETTINGS_COMPILER_VERSION
  #CONAN_SETTINGS_OS ("Linux","Windows","Macos")

  set(NAME_STUB "${CONAN_INCLUDEOS_ROOT}/src/service_name.cpp")
  set(CRTN ${CONAN_LIB_DIRS_MUSL}/crtn.o)
  set(CRTI ${CONAN_LIB_DIRS_MUSL}/crti.o)
  if (NOT DEFINED ARCH)
    if (${CONAN_SETTINGS_ARCH} STREQUAL "x86")
      set(ARCH i686)
    else()
      set(ARCH ${CONAN_SETTINGS_ARCH})
    endif()
  endif()
  set(LIBRARIES ${CONAN_LIBS})
  set(ELF_SYMS elf_syms)
  set(LINK_SCRIPT ${CONAN_INCLUDEOS_ROOT}/${ARCH}/linker.ld)
else()
  #TODO initialise self
  #message(FATAL_ERROR "Not running under conan")
  #TODO surely we can fix this!!
  if (NOT DEFINED ARCH)
    if (DEFINED ENV{ARCH})
      set(ARCH $ENV{ARCH})
    else()
      set(ARCH x86_64)
    endif()
  endif()

  include_directories(
    ${INCLUDEOS_PREFIX}/${ARCH}/include/c++/v1
    ${INCLUDEOS_PREFIX}/${ARCH}/include/c++/v1/experimental
    ${INCLUDEOS_PREFIX}/${ARCH}/include
    ${INCLUDEOS_PREFIX}/include/os
  )

  set(NAME_STUB "${INCLUDEOS_PREFIX}/src/service_name.cpp")
  set(CRTN ${INCLUDEOS_PREFIX}/${ARCH}/lib/crtn.o)
  set(CRTI ${INCLUDEOS_PREFIX}/${ARCH}/lib/crti.o)
  #TODO do the whole ye old dance

  add_library(libos STATIC IMPORTED)
  set_target_properties(libos PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(libos PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libos.a)

  add_library(libarch STATIC IMPORTED)
  set_target_properties(libarch PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(libarch PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libarch.a)

  add_library(libplatform STATIC IMPORTED)
  set_target_properties(libplatform PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(libplatform PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/platform/lib${PLATFORM}.a)

  if(${ARCH} STREQUAL "x86_64")
    add_library(libbotan STATIC IMPORTED)
    set_target_properties(libbotan PROPERTIES LINKER_LANGUAGE CXX)
    set_target_properties(libbotan PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libbotan-2.a)

    add_library(libs2n STATIC IMPORTED)
    set_target_properties(libs2n PROPERTIES LINKER_LANGUAGE CXX)
    set_target_properties(libs2n PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libs2n.a)

    add_library(libssl STATIC IMPORTED)
    set_target_properties(libssl PROPERTIES LINKER_LANGUAGE CXX)
    set_target_properties(libssl PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libssl.a)

    add_library(libcrypto STATIC IMPORTED)
    set_target_properties(libcrypto PROPERTIES LINKER_LANGUAGE CXX)
    set_target_properties(libcrypto PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libcrypto.a)
    set(OPENSSL_LIBS libs2n libssl libcrypto)

    include_directories(${INSTALL_LOC}/${ARCH}/include)
  endif()
  if (NOT ${PLATFORM} STREQUAL x86_nano )
    add_library(http_parser STATIC IMPORTED)
    set_target_properties(http_parser PROPERTIES LINKER_LANGUAGE CXX)
    set_target_properties(http_parser PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/http_parser.o)

    add_library(uzlib STATIC IMPORTED)
    set_target_properties(uzlib PROPERTIES LINKER_LANGUAGE CXX)
    set_target_properties(uzlib PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libtinf.a)

  endif()

  add_library(musl_syscalls STATIC IMPORTED)
  set_target_properties(musl_syscalls PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(musl_syscalls PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libmusl_syscalls.a)

  add_library(libcxx STATIC IMPORTED)
  add_library(cxxabi STATIC IMPORTED)
  add_library(libunwind STATIC IMPORTED)

  set_target_properties(libcxx PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(libcxx PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libc++.a)
  set_target_properties(cxxabi PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(cxxabi PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libc++abi.a)
  set_target_properties(libunwind PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(libunwind PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libunwind.a)

  add_library(libc STATIC IMPORTED)
  set_target_properties(libc PROPERTIES LINKER_LANGUAGE C)
  set_target_properties(libc PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libc.a)

  add_library(libpthread STATIC IMPORTED)
  set_target_properties(libpthread PROPERTIES LINKER_LANGUAGE C)
  set_target_properties(libpthread PROPERTIES IMPORTED_LOCATION "${INCLUDEOS_PREFIX}/${ARCH}/lib/libpthread.a")

  # libgcc/compiler-rt detection
  if (UNIX)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(TARGET_LINE --target=${TRIPLE})
    endif()
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} ${TARGET_LINE} --print-libgcc-file-name
        RESULT_VARIABLE CC_RT_RES
        OUTPUT_VARIABLE COMPILER_RT_FILE OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (NOT ${CC_RT_RES} EQUAL 0)
      message(AUTHOR_WARNING "Failed to detect libgcc/compiler-rt: ${COMPILER_RT_FILE}")
    endif()
  endif()
  if (NOT COMPILER_RT_FILE)
    set(COMPILER_RT_FILE "${INCLUDEOS_PREFIX}/${ARCH}/lib/libcompiler.a")
  endif()

  add_library(libgcc STATIC IMPORTED)
  set_target_properties(libgcc PROPERTIES LINKER_LANGUAGE C)
  set_target_properties(libgcc PROPERTIES IMPORTED_LOCATION "${COMPILER_RT_FILE}")

  if ("${PLATFORM}" STREQUAL "x86_solo5")
    add_library(solo5 STATIC IMPORTED)
    set_target_properties(solo5 PROPERTIES LINKER_LANGUAGE C)
    set_target_properties(solo5 PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/solo5_hvt.o)
  endif()

  if (${PLATFORM} STREQUAL x86_nano)
    set(LIBRARIES
      libos
      libplatform
      libarch
      musl_syscalls
      libc
      libcxx
      libunwind
      libpthread
      libgcc
      ${LIBR_CMAKE_NAMES}
    )

  else()
    set(LIBRARIES
      libgcc
      libplatform
      libarch

      ${LIBR_CMAKE_NAMES}
      libos
      libbotan
      ${OPENSSL_LIBS}
      musl_syscalls
      libcxx
      libunwind
      libpthread
      libc
      libgcc
    )
  endif()

  set(ELF_SYMS ${INCLUDEOS_PREFIX}/bin/elf_syms)
  set(LINK_SCRIPT ${INCLUDEOS_PREFIX}/${ARCH}/linker.ld)
endif()



# configure options
option(default_stdout "Use the OS default stdout (serial)" ON)

option(debug "Build with debugging symbols (OBS: increases binary size)" OFF)
option(minimal "Build for minimal size" OFF)
option(stripped "Strip symbols to further reduce size" OFF)

option(smp "Enable SMP (multiprocessing)" OFF)
option(undefined_san "Enable undefined-behavior sanitizer" OFF)
option(thin_lto "Enable Thin LTO plugin" OFF)
option(full_lto "Enable full LTO (also works on LD)" OFF)
option(coroutines "Compile with coroutines TS support" OFF)

# arch and platform defines
message(STATUS "Building for arch ${ARCH}, platform ${PLATFORM}")
set(TRIPLE "${ARCH}-pc-linux-elf")
set(CMAKE_CXX_COMPILER_TARGET ${TRIPLE})
set(CMAKE_C_COMPILER_TARGET ${TRIPLE})



add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
add_definitions(-DPLATFORM="${PLATFORM}")
add_definitions(-DPLATFORM_${PLATFORM})

# Arch-specific defines & options
if ("${ARCH}" STREQUAL "x86_64")
  set(ARCH_INTERNAL "ARCH_X64")
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf64")
  set(OBJCOPY_TARGET "elf64-x86-64")
#  set(CAPABS "${CAPABS} -m64")
else()
  set(ARCH_INTERNAL "ARCH_X86")
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf")
  set(OBJCOPY_TARGET "elf32-i386")
#  set(CAPABS "${CAPABS} -m32")
endif()

enable_language(ASM_NASM)

set(ELF ${ARCH})
if (${ELF} STREQUAL "i686")
  set(ELF "i386")
endif()

set(PRE_BSS_SIZE  "--defsym PRE_BSS_AREA=0x0")
if ("${PLATFORM}" STREQUAL "x86_solo5")
  # pre-BSS memory hole for uKVM global variables
  set(PRE_BSS_SIZE  "--defsym PRE_BSS_AREA=0x200000")
endif()

# linker stuff
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS) # this removed -rdynamic from linker output
if (CMAKE_BUILD_TYPE MATCHES DEBUG)
  set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> -o <TARGET> <LINK_FLAGS> <OBJECTS> ${CRTI} --start-group <LINK_LIBRARIES> --end-group ${CRTN}")
else()
  set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> -S -o <TARGET> <LINK_FLAGS> <OBJECTS> ${CRTI} --start-group <LINK_LIBRARIES> --end-group ${CRTN}")
endif()

set(CMAKE_SKIP_RPATH ON)
set(BUILD_SHARED_LIBRARIES OFF)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

#TODO find a more proper way to get the linker.ld script ?
set(LDFLAGS "-nostdlib -melf_${ELF} --eh-frame-hdr ${LD_STRIP} --script=${LINK_SCRIPT}  ${PRE_BSS_SIZE}")





#set(LDFLAGS "-nostdlib ${LINK_FLAGS}")
set(ELF_POSTFIX .elf.bin)

#TODO fix so that we can add two executables in one service (NAME_STUB)
function(os_add_executable TARGET NAME)
  set(ELF_TARGET ${TARGET}${ELF_POSTFIX})
  add_executable(${ELF_TARGET} ${ARGN} ${NAME_STUB})
  set_property(SOURCE ${NAME_STUB} PROPERTY COMPILE_DEFINITIONS SERVICE="${TARGET}" SERVICE_NAME="${NAME}")

  set_target_properties(${ELF_TARGET} PROPERTIES LINK_FLAGS ${LDFLAGS})
  #target_link_libraries(${ELF_TARGET} ${CRTI})
  target_link_libraries(${ELF_TARGET} ${LIBRARIES})
  #target_link_libraries(${ELF_TARGET} ${CRTN})

  #add_custom_target(TARGET _elf_symbols.bin
  #  DEPEN
  #TODO if not debug strip
  if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(STRIP_LV )
  else()
    set(STRIP_LV strip --strip-all ${CMAKE_BINARY_DIR}/${TARGET})
  endif()
  FILE(WRITE ${CMAKE_BINARY_DIR}/binary.txt
    "${TARGET}"
  )
  add_custom_target(
    ${TARGET} ALL
    COMMENT "elf.syms"
    COMMAND ${ELF_SYMS} $<TARGET_FILE:${ELF_TARGET}>
    COMMAND ${CMAKE_OBJCOPY} --update-section .elf_symbols=_elf_symbols.bin  $<TARGET_FILE:${ELF_TARGET}> ${CMAKE_BINARY_DIR}/${TARGET}
    COMMAND ${STRIP_LV}
    DEPENDS ${ELF_TARGET}
  )

  if (CONFIG_JSON)
    os_add_config(${ELF_TARGET} "config.json")
   endif()

endfunction()

##


function(os_link_libraries TARGET)
  target_link_libraries(${TARGET}${ELF_POSTFIX} ${ARGN})
endfunction()

function (os_add_library_from_path TARGET LIBRARY PATH)
  set(FILE_NAME "${PATH}/lib${LIBRARY}.a")

  if(NOT EXISTS ${FILE_NAME})
    message(FATAL_ERROR "Library lib${LIBRARY}.a not found at ${PATH}")
    return()
  endif()

  add_library(${LIBRARY} STATIC IMPORTED)
  set_target_properties(${LIBRARY} PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(${LIBRARY} PROPERTIES IMPORTED_LOCATION ${FILE_NAME})
  os_link_libraries(${TARGET}  --whole-archive ${LIBRARY} --no-whole-archive)
endfunction()

function (os_add_drivers TARGET)
  foreach(DRIVER ${ARGN})
    os_add_library_from_path(${TARGET} ${DRIVER} "${INCLUDEOS_PREFIX}/${ARCH}/drivers")
  endforeach()
endfunction()

function(os_add_plugins TARGET)
  foreach(PLUGIN ${ARGN})
    os_add_library_from_path(${TARGET} ${PLUGIN} "${INCLUDEOS_PREFIX}/${ARCH}/plugins")
  endforeach()
endfunction()

function (os_add_stdout TARGET DRIVER)
   os_add_library_from_path(${TARGET} ${DRIVER} "${INCLUDEOS_PREFIX}/${ARCH}/drivers/stdout")
endfunction()

function(os_add_os_library TARGET)
   message(FATAL_ERROR "Function not implemented yet.. pull from conan or install to lib ?")
  #target_link_libraries(${TARGET}${ELF_POSTFIX} ${ARGN})
endfunction()



#TODO investigate could be wrapped in generic embed object ?
#If the user sets the config.json in the CMAKE then at least he knows its inluded :)
#If the user sets the nacl in CMAKE thats also specific..
#so the idea is..
#SET(OS_NACL ON)
#SET(OS_CONFIG ON)
#SET(OS_NACL_FILE
#SET(OS_CONFIG_FILE

#Investigate how to add drivers for a service !!
#idea is to have a conanfile.txt in the service you edit..
#this depends on a generic conanfile_service.py ?
#if so you can edit plugins and such in that file..

function(os_add_config TARGET CONFIG_JSON)
  set(OUTFILE ${CMAKE_BINARY_DIR}/${CONFIG_JSON}.o)
  add_custom_command(
    OUTPUT ${OUTFILE}
    COMMAND ${CMAKE_OBJCOPY} -I binary -O ${OBJCOPY_TARGET} -B i386 --rename-section .data=.config,CONTENTS,ALLOC,LOAD,READONLY,DATA ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_JSON} ${OUTFILE}
    DEPENDS ${CONFIG_JSON}
    )
  add_library(config_json STATIC ${OUTFILE})
  set_target_properties(config_json PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(${TARGET}${TARGET_POSTFIX} --whole-archive config_json --no-whole-archive)
endfunction()




function(os_install)
  set(options OPTIONAL)
  set(oneValueArgs DESTINATION)
  set(multiValueArgs TARGETS)
  cmake_parse_arguments(os_install "${optional}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (os_install_DESTINATION STREQUAL "")
    set(os_install_DESTINATION bin)
  endif()

  foreach(T ${os_install_TARGETS})
    #message("OS install  ${T} to ${os_install_DESTINATION}")
    install(PROGRAMS ${CMAKE_BINARY_DIR}/${T} DESTINATION ${os_install_DESTINATION})
  endforeach()

endfunction()

if(CONAN_EXPORTED)
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
else()
  #TODO initialise self
  message(FATAL_ERROR "Not running under conan")
endif()

#TODO use these
#CONAN_SETTINGS_ARCH Provides arch type
#CONAN_SETTINGS_BUILD_TYPE provides std cmake "Debug" and "Release"
#CONAN_SETTINGS_COMPILER AND CONAN_SETTINGS_COMPILER_VERSION
#CONAN_SETTINGS_OS ("Linux","Windows","Macos")

if (NOT DEFINED ARCH)
  if (DEFINED ENV{ARCH})
    set(ARCH $ENV{ARCH})
  else()
    set(ARCH x86_64)
  endif()
endif()

if (NOT DEFINED PLATFORM)
  if (DEFINED ENV{PLATFORM})
    set(PLATFORM $ENV{PLATFORM})
  else()
    set(PLATFORM x86_pc)
  endif()
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

set(CPP_VERSION c++17)

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
add_definitions(-DPLATFORM="${PLATFORM}")
add_definitions(-DPLATFORM_${PLATFORM})

set(CRTN ${CONAN_LIB_DIRS_MUSL}/crtn.o)
set(CRTI ${CONAN_LIB_DIRS_MUSL}/crti.o)

#list(GET CONAN_LIB_DIRS_INCLUDEOS 1 INCLUDEOS_LIBS)


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
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> -o <TARGET> <LINK_FLAGS> <OBJECTS> --start-group <LINK_LIBRARIES> --end-group ")
set(CMAKE_SKIP_RPATH ON)
set(BUILD_SHARED_LIBRARIES OFF)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

#TODO find a more proper way to get the linker.ld script ?
set(LDFLAGS "-nostdlib -melf_${ELF} --eh-frame-hdr ${LD_STRIP} --script=${CONAN_INCLUDEOS_ROOT}/${ARCH}/linker.ld ${PRE_BSS_SIZE}")

set(NAME_STUB "${CONAN_INCLUDEOS_ROOT}/src/service_name.cpp")

set(LIBRARIES ${CONAN_LIBS})
#set(LDFLAGS "-nostdlib ${LINK_FLAGS}")
set(ELF_POSTFIX .elf.bin)
function(os_add_executable TARGET NAME)


  ##get name stub in service..
  set(ELF_TARGET ${TARGET}${ELF_POSTFIX})
  add_executable(${ELF_TARGET} ${ARGN} ${NAME_STUB})
  set_property(SOURCE ${NAME_STUB} PROPERTY COMPILE_DEFINITIONS SERVICE="${TARGET}" SERVICE_NAME="${NAME}")
  set_target_properties(${ELF_TARGET} PROPERTIES LINK_FLAGS ${LDFLAGS})
  target_link_libraries(${ELF_TARGET} ${CRTI})
  target_link_libraries(${ELF_TARGET} ${LIBRARIES})
  target_link_libraries(${ELF_TARGET} ${CRTN})

  #add_custom_target(TARGET _elf_symbols.bin
  #  DEPEN

 # add_custom_command(
  #  TARGET ${CMAKE_BINARY_DIR}/${TARGET}
   # POST_BUILD
   # COMMAND elf_syms $<TARGET_FILE:${ELF_TARGET}>
   # COMMAND ${CMAKE_OBJCOPY} --update-section .elf_symbols=_elf_symbols.bin  $<TARGET_FILE:${ELF_TARGET}> ${CMAKE_BINARY_DIR}/${TARGET}
   # COMMAND ${STRIP_LV}
   # DEPENDS ${ELF_TARGET}
   # )

  add_custom_target(
    ${TARGET} ALL
    COMMENT "elf.syms"
    COMMAND elf_syms $<TARGET_FILE:${ELF_TARGET}>
    COMMAND ${CMAKE_OBJCOPY} --update-section .elf_symbols=_elf_symbols.bin  $<TARGET_FILE:${ELF_TARGET}> ${CMAKE_BINARY_DIR}/${TARGET}
    COMMAND ${STRIP_LV}
    DEPENDS ${ELF_TARGET})
  #add_custom_target(${TARGET}
  #  pruned_elf_symbols ALL
  #  COMMAND elf_syms ${BINARY}
  #  COMMAND ${CMAKE_OBJCOPY} --update-section .elf_symbols=_elf_symbols.bin ${TARGET}${TARGET_POSTFIX} ${TARGET}
  #  COMMAND ${STRIP_LV}
  #  DEPENDS ${TARGET}${TARGET_POSTFIX}
  #)
endfunction()

##


function(os_link_libraries target)
  target_link_libraries(${TARGET}${TARGET_POSTFIX} ${ARGN})
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
  set(OUTFILE {CONFIG_JSON}.o)
  add_custom_command(
    OUTPUT ${OUTFILE}
    COMMAND ${CMAKE_OBJCOPY} -I binary -O -B i386 --rename-section .data=.config,CONTENTS,ALLOC,LOAD,READONLY,DATA ${CONFIG_JSON} ${OUTFILE}
    DEPENDS ${CONFIG_JSON}
    )
  add_library(config_json STATIC ${OUTFILE})
  #ADD this as a string to targets ?
  target_link_libraries(${TARGET}${TARGET_POSTFIX} --whole-archive config_json --no-whole-archive)
  #add autoconf plugin ?

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

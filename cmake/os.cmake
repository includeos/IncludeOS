if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)


if (NOT INCLUDEOS_PACKAGE)
  message(FATAL_ERROR "INCLUDEOS_PACKAGE is not set")
endif()

if (NOT ARCH)
  message(FATAL_ERROR "ARCH is not set")
endif()

find_program(PYTHON3_EXECUTABLE python3)
if (PYTHON3_EXECUTABLE-NOTFOUND)
  message(FATAL_ERROR "python3 not found")
endif()

if (NOT DEFINED PLATFORM)
  if (DEFINED ENV{PLATFORM})
    set(PLATFORM $ENV{PLATFORM})
  else()
    set(PLATFORM x86_pc)
  endif()
endif()


set(NAME_STUB "${INCLUDEOS_PACKAGE}/src/service_name.cpp")

set(TRIPLE "${ARCH}-pc-linux-elf")


if (ELF_SYMBOLS)
  find_program(ELF_SYMS elf_syms HINTS ${INCLUDEOS_PACKAGE}/tools/vmbuild)
  if (ELF_SYMS-NOTFOUND)
    message(FATAL_ERROR "elf_syms not found")
  endif()
endif()

find_program(DISKBUILDER diskbuilder)
if (DISKBUILDER-NOTFOUND)
  message(FATAL_ERROR "diskbuilder not found")
endif()

set(LINK_SCRIPT ${INCLUDEOS_PACKAGE}/linker.ld)

include_directories(
  ${INCLUDEOS_PACKAGE}/include/os
)

# Arch and platform defines
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
elseif("${ARCH}" STREQUAL "aarch64")

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
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> -o <TARGET> <LINK_FLAGS> <OBJECTS> ${CRTI} --start-group <LINK_LIBRARIES> --end-group ${CRTN}")
else()
  set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> -S -o <TARGET> <LINK_FLAGS> <OBJECTS> ${CRTI} --start-group <LINK_LIBRARIES> --end-group ${CRTN}")
endif()

set(CMAKE_SKIP_RPATH ON)
set(BUILD_SHARED_LIBRARIES OFF)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

option(FOR_PRODUCTION "Stop the OS when conditions not suitable for production" OFF)
if (FOR_PRODUCTION)
  set(PROD_USE "--defsym __for_production_use=0x2000")
else()
  set(PROD_USE "--defsym __for_production_use=0x1000")
endif()

# TODO: find a more proper way to get the linker.ld script ?
set(LD_COMMON "-nostdlib --eh-frame-hdr ${LD_STRIP} --script=${LINK_SCRIPT} ${PROD_USE}")
set(LD_COMMON "${LD_COMMON} --gc-sections $ENV{NIX_LDFLAGS}") # TODO: DON'T DO NIX LIKE THIS

if("${ARCH}" STREQUAL "aarch64")
  set(LDFLAGS "-nostdlib -m${ELF}elf --eh-frame-hdr ${LD_STRIP} --script=${LINK_SCRIPT} ${PROD_USE} ${PRE_BSS_SIZE}")
else()
  set(LDFLAGS "-nostdlib -melf_${ELF} --eh-frame-hdr ${LD_STRIP} --script=${LINK_SCRIPT} ${PROD_USE} ${PRE_BSS_SIZE}")
endif()

set(ELF_POSTFIX .elf.bin)

SET(DEFAULT_CONFIG_JSON ${CMAKE_CURRENT_SOURCE_DIR}/config.json)

function(os_add_config TARGET FILE)
  set(ELF_TARGET ${TARGET}${ELF_POSTFIX})
  message(STATUS "adding config file ${FILE}")
  if (DEFINED JSON_CONFIG_FILE_${ELF_TARGET})
    message(FATAL_ERROR "config already set to ${JSON_CONFIG_FILE_${ELF_TARGET}} add os_add_config prior to os_add_executable")
  endif()
  set(JSON_CONFIG_FILE_${ELF_TARGET} ${FILE} PARENT_SCOPE)
endfunction()

function(os_add_executable TARGET NAME)
  set(ELF_TARGET ${TARGET}${ELF_POSTFIX})
  add_executable(${ELF_TARGET} ${ARGN} ${NAME_STUB})

  set_property(SOURCE ${NAME_STUB} PROPERTY COMPILE_DEFINITIONS
            SERVICE="${TARGET}" SERVICE_NAME="${NAME}"
            _LIVEUPDATE_MEMSIZE_=${LIVEUPDATE_MB})

  target_compile_options(${ELF_TARGET} PRIVATE -Wall -Wextra -fstack-protector)
  target_compile_options(${ELF_TARGET} PRIVATE -ffunction-sections -fdata-sections)
  if (DISABLE_SYSTEM_PATHS)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      message(STATUS "Adding clang compile options, -nostdlib, nostdlibinc")
      target_compile_options(${ELF_TARGET} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-nostdlib -nostdlibinc>)
    else()
      message(STATUS "Adding compile options, -nostdlib, nostdinc")
      target_compile_options(${ELF_TARGET} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-nostdlib -nostdinc>)
    endif()
  endif()

  if (PROFILE)
	  target_compile_definitions(${ELF_TARGET} PRIVATE ENABLE_PROFILERS=1)
  endif()

  set_target_properties(${ELF_TARGET} PROPERTIES LINK_FLAGS ${LDFLAGS})

  if (${PLATFORM} STREQUAL "x86_pc")
    set(LIBPLATFORM lib${ARCH}_pc.a)
  elseif (${PLATFORM} STREQUAL "nano")
    set(LIBPLATFORM lib${ARCH}_nano.a)
  else()
    set(LIBPLATFORM lib${ARCH}_${PLATFORM}.a)
  endif()

  if (${ARCH} STREQUAL "i686")
    set(LIBGCC libclang_rt.builtins-i386.a)
  else()
    set(LIBGCC libclang_rt.builtins-${ARCH}.a)
  endif()

  set(LIBRARIES
    ${INCLUDEOS_PACKAGE}/lib/libos.a
    ${INCLUDEOS_PACKAGE}/platform/${LIBPLATFORM}
    ${INCLUDEOS_PACKAGE}/lib/libarch.a
    ${INCLUDEOS_PACKAGE}/lib/libos.a
    ${INCLUDEOS_PACKAGE}/libcxx/lib/libc++.a
    ${INCLUDEOS_PACKAGE}/libc/lib/libc.a
    ${INCLUDEOS_PACKAGE}/lib/libmusl_syscalls.a
    ${INCLUDEOS_PACKAGE}/libunwind/lib/libunwind.a
    ${INCLUDEOS_PACKAGE}/libgcc/lib/linux/${LIBGCC}
  )

  message(STATUS ">>>>> 👉 Libraries: ${LIBRARIES}")
  foreach(_LIB ${LIBRARIES})
    message(STATUS ">>>>> 👉 Adding library ${_LIB}")
    get_filename_component(_PATH ${_LIB} DIRECTORY)
    if (_PATH MATCHES ".*drivers" OR _PATH MATCHES ".*plugins" OR _PATH MATCHES ".*stdout")
      message(STATUS "Whole Archive " ${_LIB})
      os_link_libraries(${TARGET} --whole-archive ${_LIB} --no-whole-archive)
    else()
      target_link_libraries(${ELF_TARGET} ${_LIB})
    endif()
  endforeach()

  # TODO: if not debug strip
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(STRIP_LV )
  else()
    set(STRIP_LV ${CMAKE_STRIP} --strip-all ${CMAKE_CURRENT_BINARY_DIR}/${TARGET})
  endif()
  FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/binary.txt
    "${TARGET}"
  )
  if (ELF_SYMBOLS)
      add_custom_target(
        ${TARGET} ALL
        COMMENT "elf.syms"
        COMMAND ${ELF_SYMS} $<TARGET_FILE:${ELF_TARGET}>
        # Copy symbols to .elf_symbols section
        COMMAND ${CMAKE_OBJCOPY} --update-section .elf_symbols=_elf_symbols.bin  $<TARGET_FILE:${ELF_TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}
        # Strip regular symbols
        COMMAND ${CMAKE_STRIP} --strip-all ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}
        COMMAND mv $<TARGET_FILE:${ELF_TARGET}> $<TARGET_FILE:${ELF_TARGET}>.copy
        DEPENDS ${ELF_TARGET}
      )
  else()
    # TODO: Re-enable stripping.
    # They won't be used inside IncludeOS as-is, but are likely in the way of
    # something else, like .bss.
    # Restoring ELF_SYMBOLS should be done first though.
    #add_custom_target(
    #    ${TARGET} ALL
    #    COMMAND cp bin/${ELF_TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}
    #    COMMAND ${STRIP_CMD} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}
    #    DEPENDS ${ELF_TARGET}
    #  )
  endif()

  if (DEFINED JSON_CONFIG_FILE_${ELF_TARGET})
    message(STATUS "using set config file ${JSON_CONFIG_FILE_${ELF_TARGET}}")
    internal_os_add_config(${ELF_TARGET} "${JSON_CONFIG_FILE_${ELF_TARGET}}")
  elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.json)
    message(STATUS "using detected config file ${CMAKE_CURRENT_SOURCE_DIR}/config.json")
    internal_os_add_config(${ELF_TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/config.json")
    set(JSON_CONFIG_FILE_${ELF_TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/config.json" PARENT_SCOPE)
  endif()
  #copy the vm.json out of tree
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/vm.json)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vm.json ${CMAKE_CURRENT_BINARY_DIR})
  endif()
endfunction()

function(os_compile_options TARGET)
  target_compile_options(${TARGET}${ELF_POSTFIX} ${ARGN})
endfunction()

function(os_compile_definitions TARGET)
  target_link_libraries(${TARGET}${ELF_POSTFIX} ${ARGN})
endfunction()

function(os_link_libraries TARGET)
  target_link_libraries(${TARGET}${ELF_POSTFIX} ${ARGN})
endfunction()

function(os_include_directories TARGET)
  message(STATUS "Including directories ${TARGET}${ELF_POSTFIX} ${ARGN}")
  target_include_directories(${TARGET}${ELF_POSTFIX} ${ARGN})
endfunction()

function(os_add_dependencies TARGET ${ARGN})
  add_dependencies(${TARGET}${ELF_POSTFIX} ${ARGN})
endfunction()

function (os_add_library_from_path TARGET LIBRARY PATH)
  set(FILENAME "${PATH}/lib${LIBRARY}.a")

  if(NOT EXISTS ${FILENAME})
    message(FATAL_ERROR "Library lib${LIBRARY}.a not found at ${PATH}")
    return()
  endif()

  add_library(${TARGET}_${LIBRARY} STATIC IMPORTED)
  set_target_properties(${TARGET}_${LIBRARY} PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(${TARGET}_${LIBRARY} PROPERTIES IMPORTED_LOCATION "${FILENAME}")
  os_link_libraries(${TARGET} --whole-archive ${TARGET}_${LIBRARY} --no-whole-archive)
endfunction()

function (os_add_drivers TARGET)
  foreach(DRIVER ${ARGN})
    #if in conan expect it to be in order ?
    os_add_library_from_path(${TARGET} ${DRIVER} "${INCLUDEOS_PACKAGE}/drivers")
  endforeach()
endfunction()

function(os_add_plugins TARGET)
  foreach(PLUGIN ${ARGN})
    os_add_library_from_path(${TARGET} ${PLUGIN} "${INCLUDEOS_PACKAGE}/plugins")
  endforeach()
endfunction()

function (os_add_stdout TARGET DRIVER)
   os_add_library_from_path(${TARGET} ${DRIVER} "${INCLUDEOS_PACKAGE}/drivers/stdout")
endfunction()


#input file blob name and blob type eg add_binary_blob(<somefile> input.bin binary)
#results in an object called binary_input_bin
function(os_add_binary_blob TARGET BLOB_FILE BLOB_NAME BLOB_TYPE)
  set(OBJECT_FILE ${TARGET}_blob_${BLOB_TYPE}.o)
  add_custom_command(
    OUTPUT ${OBJECT_FILE}
    COMMAND cp ${BLOB_FILE} ${BLOB_NAME}
    COMMAND ${CMAKE_OBJCOPY} -I ${BLOB_TYPE} -O ${OBJCOPY_TARGET} -B i386 ${BLOB_NAME} ${OBJECT_FILE}
    COMMAND rm ${BLOB_NAME}
    DEPENDS ${BLOB_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

  add_library(${TARGET}_blob_${BLOB_TYPE} STATIC ${OBJECT_FILE})
  set_target_properties(${TARGET}_blob_${BLOB_TYPE} PROPERTIES LINKER_LANGUAGE CXX)
  os_link_libraries(${TARGET} --whole-archive ${TARGET}_blob_${BLOB_TYPE} --no-whole-archive)
endfunction()
# add memdisk
function(os_add_memdisk TARGET DISK)
  get_filename_component(DISK_RELPATH "${DISK}"
    REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  add_custom_command(
    OUTPUT  memdisk.o
    COMMAND ${PYTHON3_EXECUTABLE} ${INCLUDEOS_PACKAGE}/tools/memdisk/memdisk.py --file memdisk.asm ${DISK_RELPATH}
    COMMAND nasm -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} memdisk.asm -o memdisk.o
    DEPENDS ${DISK}
  )
  add_library(${TARGET}_memdisk STATIC memdisk.o)
  set_target_properties(${TARGET}_memdisk PROPERTIES LINKER_LANGUAGE CXX)
  os_link_libraries(${TARGET} --whole-archive ${TARGET}_memdisk --no-whole-archive)
endfunction()

# automatically build memdisk from folder
function(os_build_memdisk TARGET FOLD)
  get_filename_component(REL_PATH "${FOLD}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  #detect changes in disc folder and if and only if changed update the file that triggers rebuild
  find_program(CHSUM NAMES md5sum md5)
  if (CHSUM-NOTFOUND)
    message(FATAL_ERROR md5sum not found)
  endif()
  add_custom_target(${TARGET}_disccontent ALL
    COMMAND find ${REL_PATH}/ -type f -exec ${CHSUM} "{}" + > /tmp/manifest.txt.new
    COMMAND cmp --silent ${CMAKE_CURRENT_BINARY_DIR}/manifest.txt /tmp/manifest.txt.new || cp /tmp/manifest.txt.new ${CMAKE_CURRENT_BINARY_DIR}/manifest.txt
    COMMENT "Checking disc content changes"
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/manifest.txt
    VERBATIM
  )

  add_custom_command(
      OUTPUT  memdisk.fat
      COMMAND ${DISKBUILDER} -o memdisk.fat ${REL_PATH}
      COMMENT "Creating memdisk"
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/manifest.txt ${TARGET}_disccontent
  )
  add_custom_target(${TARGET}_diskbuilder DEPENDS memdisk.fat)
  os_add_dependencies(${TARGET} ${TARGET}_diskbuilder)
  os_add_memdisk(${TARGET} "${CMAKE_CURRENT_BINARY_DIR}/memdisk.fat")
endfunction()

# call build_memdisk only if MEMDISK is not defined from command line
function(os_diskbuilder TARGET FOLD)
  os_build_memdisk(${TARGET} ${FOLD})
endfunction()

# Add both standard certificate bundle and any specific certs provided as parameters
function(os_add_ssl_certificates TARGET)
  set(CERTS_DIR ${CMAKE_CURRENT_BINARY_DIR}/certs)
  file(MAKE_DIRECTORY ${CERTS_DIR})
  foreach(CERTIFICATE ${ARGN})
    get_filename_component(CERT ${CERTIFICATE} ABSOLUTE BASE_DIR ${CMAKE_BINARY_DIR})
    message(STATUS "Adding certificate: ${CERT}")
    file(COPY ${CERT} DESTINATION ${CERTS_DIR})
  endforeach()
  message(STATUS "Downloading ssl certificates")
  file(DOWNLOAD https://github.com/fwsGonzo/s2n_bundle/releases/download/v1/ca_bundle.tar.gz ${CMAKE_CURRENT_BINARY_DIR}/certs.tgz)
  execute_process(
    COMMAND tar -xf  ${CMAKE_CURRENT_BINARY_DIR}/certs.tgz --strip-components=1 -C ${CERTS_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  os_build_memdisk(${TARGET} ${CMAKE_CURRENT_BINARY_DIR}/certs)
endfunction()

function(internal_os_add_config TARGET CONFIG_JSON)
  get_filename_component(FILENAME "${CONFIG_JSON}" NAME)
  set(OUTFILE ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.o)
  add_custom_command(
    OUTPUT ${OUTFILE}
    COMMAND ${CMAKE_OBJCOPY} -I binary -O ${OBJCOPY_TARGET} -B i386 --rename-section .data=.config,CONTENTS,ALLOC,LOAD,READONLY,DATA ${CONFIG_JSON} ${OUTFILE}
    DEPENDS ${CONFIG_JSON}
    )
  add_library(config_json_${TARGET} STATIC ${OUTFILE})
  set_target_properties(config_json_${TARGET} PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(${TARGET}${TARGET_POSTFIX} --whole-archive config_json_${TARGET} --no-whole-archive)
endfunction()

function(os_add_nacl TARGET FILENAME)
  find_program(PYTHON2_EXECUTABLE python)
  if (PYTHON2_EXECUTABLE-NOTFOUND)
    message(FATAL_ERROR "Python not found")
  endif()
  find_program(NACL_SCRIPT NaCl.py)
  if (NACL_SCRIPT-NOTFOUND)
    message(FATAL_ERROR "NaCl.py not found")
  endif()
  add_custom_command(
     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nacl_content.cpp
     COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME} | ${PYTHON2_EXECUTABLE} ${NACL_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/nacl_content.cpp
     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}
   )
   add_library(nacl_content STATIC ${CMAKE_CURRENT_BINARY_DIR}/nacl_content.cpp)
   set_target_properties(nacl_content PROPERTIES LINKER_LANGUAGE CXX)
   os_link_libraries(${TARGET} --whole-archive nacl_content --no-whole-archive)
   os_add_plugins(${TARGET} nacl)
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
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${T} DESTINATION ${os_install_DESTINATION})
  endforeach()

endfunction()

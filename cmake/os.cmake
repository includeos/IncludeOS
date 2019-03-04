
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
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

if (CONAN_EXPORTED OR CONAN_LIBS)
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  if (CONAN_EXPORTED)
    include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
    #hack for editable package
    set(INCLUDEOS_PREFIX ${CONAN_INCLUDEOS_ROOT})
  else()
    #hack for editable package
    set(INCLUDEOS_PREFIX ${CONAN_INCLUDEOS_ROOT}/install)
  endif()


  #TODO use these
  #CONAN_SETTINGS_ARCH Provides arch type
  #CONAN_SETTINGS_BUILD_TYPE provides std cmake "Debug" and "Release" "are they set by conan_basic?"
  #CONAN_SETTINGS_COMPILER AND CONAN_SETTINGS_COMPILER_VERSION
  #CONAN_SETTINGS_OS ("Linux","Windows","Macos")

  if (NOT DEFINED ARCH)
    if (${CONAN_SETTINGS_ARCH} STREQUAL "x86")
      set(ARCH i686)
    else()
      set(ARCH ${CONAN_SETTINGS_ARCH})
    endif()
  endif()


  set(NAME_STUB "${CONAN_INCLUDEOS_ROOT}/src/service_name.cpp")
  set(CRTN ${CONAN_LIB_DIRS_MUSL}/crtn.o)
  set(CRTI ${CONAN_LIB_DIRS_MUSL}/crti.o)

  set(TRIPLE "${ARCH}-pc-linux-elf")
  set(LIBRARIES ${CONAN_LIBS})
  set(ELF_SYMS elf_syms)
  set(LINK_SCRIPT ${INCLUDEOS_PREFIX}/${ARCH}/linker.ld)
  #includeos package can provide this!
  include_directories(
    ${INCLUDEOS_PREFIX}/include/os
  )


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

  set(TRIPLE "${ARCH}-pc-linux-elf")
  include_directories(
    ${INCLUDEOS_PREFIX}/${ARCH}/include/c++/v1
    #${INCLUDEOS_PREFIX}/${ARCH}/include/c++/v1/experimental
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
  add_library(libcxx_experimental STATIC IMPORTED)

  set_target_properties(libcxx PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(libcxx PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libc++.a)
  set_target_properties(cxxabi PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(cxxabi PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libc++abi.a)
  set_target_properties(libunwind PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(libunwind PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libunwind.a)
  set_target_properties(libcxx_experimental PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(libcxx_experimental PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libc++experimental.a)

  add_library(libc STATIC IMPORTED)
  set_target_properties(libc PROPERTIES LINKER_LANGUAGE C)
  set_target_properties(libc PROPERTIES IMPORTED_LOCATION ${INCLUDEOS_PREFIX}/${ARCH}/lib/libc.a)

  add_library(libpthread STATIC IMPORTED)
  set_target_properties(libpthread PROPERTIES LINKER_LANGUAGE C)
  set_target_properties(libpthread PROPERTIES IMPORTED_LOCATION "${INCLUDEOS_PREFIX}/${ARCH}/lib/libpthread.a")

  #allways use the provided libcompiler.a
  set(COMPILER_RT_FILE "${INCLUDEOS_PREFIX}/${ARCH}/lib/libcompiler.a")


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
      http_parser
      uzlib
      libbotan
      ${OPENSSL_LIBS}
      musl_syscalls
      libcxx
      libunwind
      libpthread
      libc
      libgcc
      libcxx_experimental
    )
  endif()

  set(ELF_SYMS ${INCLUDEOS_PREFIX}/bin/elf_syms)
  set(LINK_SCRIPT ${INCLUDEOS_PREFIX}/${ARCH}/linker.ld)
endif()

#TODO get the TARGET executable from diskimagebuild
if (CONAN_TARGETS)
  #find_package(diskimagebuild)
  set(DISKBUILDER diskbuilder)
else()
  set(DISKBUILDER ${INCLUDEOS_PREFIX}/bin/diskbuilder)
endif()
# arch and platform defines
#message(STATUS "Building for arch ${ARCH}, platform ${PLATFORM}")

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

# TODO: find a more proper way to get the linker.ld script ?
set(LDFLAGS "-nostdlib -melf_${ELF} --eh-frame-hdr ${LD_STRIP} --script=${LINK_SCRIPT}  ${PRE_BSS_SIZE}")


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


function(os_add_conan_package TARGET PACKAGE)

#TODO MOVE SOMEWHERE MORE SANE

  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
     message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
     file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                    "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()
  #TODO se if this goes all wack
  include(${CMAKE_BINARY_DIR}/conan.cmake)
  #should we specify a directory.. can we run it multiple times ?
  conan_cmake_run(
    REQUIRES ${PACKAGE}
    BASIC_SETUP
    CMAKE_TARGETS
  )
  #convert pkg/version@user/channel to pkg;versin;user;chanel
  string(REPLACE "@" ";" LIST ${PACKAGE})
  string(REPLACE "/" ";" LIST ${LIST})
  #get the first element
  list(GET LIST 0 PKG)

  os_link_libraries(${TARGET} CONAN_PKG::${PKG})

endfunction()

# TODO: fix so that we can add two executables in one service (NAME_STUB)
function(os_add_executable TARGET NAME)
  set(ELF_TARGET ${TARGET}${ELF_POSTFIX})
  add_executable(${ELF_TARGET} ${ARGN} ${NAME_STUB})
  set_property(SOURCE ${NAME_STUB} PROPERTY COMPILE_DEFINITIONS SERVICE="${TARGET}" SERVICE_NAME="${NAME}")

  set_target_properties(${ELF_TARGET} PROPERTIES LINK_FLAGS ${LDFLAGS})
  target_link_libraries(${ELF_TARGET} ${LIBRARIES})

  # TODO: if not debug strip
  if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(STRIP_LV )
  else()
    set(STRIP_LV ${CMAKE_STRIP} --strip-all ${CMAKE_CURRENT_BINARY_DIR}/${TARGET})
  endif()
  FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/binary.txt
    "${TARGET}"
  )
  add_custom_target(
    ${TARGET} ALL
    COMMENT "elf.syms"
    COMMAND ${ELF_SYMS} $<TARGET_FILE:${ELF_TARGET}>
    COMMAND ${CMAKE_OBJCOPY} --update-section .elf_symbols=_elf_symbols.bin  $<TARGET_FILE:${ELF_TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}
    COMMAND ${STRIP_LV}
    DEPENDS ${ELF_TARGET}
  )

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

##
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

function(os_add_os_library TARGET LIB)
  os_add_library_from_path(${TARGET} ${LIB} "${INCLUDEOS_PREFIX}/${ARCH}/lib")
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
    COMMAND ${Python2_EXECUTABLE} ${INCLUDEOS_PREFIX}/tools/memdisk/memdisk.py --file memdisk.asm ${DISK_RELPATH}
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
  add_custom_target(${TARGET}_disccontent ALL
    COMMAND find ${REL_PATH}/ -type f -exec md5sum "{}" + > /tmp/manifest.txt.new
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

function(os_install_certificates FOLDER)
  get_filename_component(REL_PATH "${FOLDER}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  message(STATUS "Install certificate bundle at ${FOLDER}")
  file(COPY ${INSTALL_LOC}/cert_bundle/ DESTINATION ${REL_PATH})
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
  set(NACL_PATH ${INCLUDEOS_PREFIX}/tools/NaCl)
  add_custom_command(
     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nacl_content.cpp
     COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME} | ${Python2_EXECUTABLE} ${NACL_PATH}/NaCl.py ${CMAKE_CURRENT_BINARY_DIR}/nacl_content.cpp
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
    #message("OS install  ${T} to ${os_install_DESTINATION}")
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${T} DESTINATION ${os_install_DESTINATION})
  endforeach()

endfunction()

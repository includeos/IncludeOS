###                                 ###
## CMakeList for IncludeOS services ##
#___________________________________#

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

set(INSTALL_LOC $ENV{INCLUDEOS_PREFIX}/includeos)

message(STATUS "Target triple ${TRIPLE}")

# defines $CAPABS depending on installation
include(${CMAKE_CURRENT_LIST_DIR}/settings.cmake)

# Arch-specific defines & options
if ("${ARCH}" STREQUAL "x86_64")
  set(ARCH_INTERNAL "ARCH_X64")
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf64")
  set(OBJCOPY_TARGET "elf64-x86-64")
  set(CAPABS "${CAPABS} -m64")
else()
  set(ARCH_INTERNAL "ARCH_X86")
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf")
  set(OBJCOPY_TARGET "elf32-i386")
  set(CAPABS "${CAPABS} -m32")
endif()
enable_language(ASM_NASM)

if (smp)
  add_definitions(-DINCLUDEOS_SMP_ENABLE)
  message(STATUS "Building with SMP enabled")
endif()

if (coroutines)
  set(CAPABS "${CAPABS} -fcoroutines-ts ")
endif()

# Various global defines
# * OS_TERMINATE_ON_CONTRACT_VIOLATION provides classic assert-like output from Expects / Ensures
# * _GNU_SOURCE enables POSIX-extensions in newlib, such as strnlen. ("everything newlib has", ref. cdefs.h)
set(CAPABS "${CAPABS} -fstack-protector-strong -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_LIBCPP_HAS_MUSL_LIBC -D_GNU_SOURCE -D__includeos__ -DSERVICE=\"\\\"${BINARY}\\\"\" -DSERVICE_NAME=\"\\\"${SERVICE_NAME}\\\"\"")
set(WARNS  "-Wall -Wextra") #-pedantic

# Compiler optimization
set(OPTIMIZE "-O2")
if (minimal)
  set(OPTIMIZE "-Os")
endif()
if (debug)
  set(CAPABS "${CAPABS} -g")
endif()

# Append sanitizers
if (undefined_san)
  set(CAPABS "${CAPABS} -fsanitize=undefined -fno-sanitize=vptr")
endif()
if (thin_lto)
  set(CMAKE_LINKER "ld.lld")
  set(OPTIMIZE "${OPTIMIZE} -flto=thin -fuse-ld=lld")
elseif (full_lto)
  set(CMAKE_LINKER "ld.lld")
  set(OPTIMIZE "${OPTIMIZE} -flto=full")
endif()

if (CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_CXX_FLAGS "-MMD ${CAPABS} ${OPTIMIZE} ${WARNS} -nostdlib -fno-omit-frame-pointer -c -std=${CPP_VERSION}")
  set(CMAKE_C_FLAGS "-MMD ${CAPABS} ${OPTIMIZE} ${WARNS} -nostdlib -fno-omit-frame-pointer -c")
else()
  # these kinda work with llvm
  set(CMAKE_CXX_FLAGS "-MMD ${CAPABS} ${OPTIMIZE} ${WARNS} -nostdlib -nostdlibinc -fno-omit-frame-pointer -c -std=${CPP_VERSION} ")
  set(CMAKE_C_FLAGS "-MMD ${CAPABS} ${OPTIMIZE} ${WARNS} -nostdlib -nostdlibinc -fno-omit-frame-pointer -c")
endif()

# executable
set(SERVICE_STUB "${INSTALL_LOC}/src/service_name.cpp")

add_executable(service ${SOURCES} ${SERVICE_STUB})
set_target_properties(service PROPERTIES OUTPUT_NAME ${BINARY})

#
# CONFIG.JSON
#

if (EXISTS ${CMAKE_SOURCE_DIR}/config.json)
  add_custom_command(
	   OUTPUT config_json.o
	   COMMAND ${CMAKE_OBJCOPY} -I binary -O ${OBJCOPY_TARGET} -B i386 --rename-section .data=.config,CONTENTS,ALLOC,LOAD,READONLY,DATA ${CMAKE_SOURCE_DIR}/config.json config_json.o
	   DEPENDS ${CMAKE_SOURCE_DIR}/config.json
   )
   add_library(config_json STATIC config_json.o)
   set_target_properties(config_json PROPERTIES LINKER_LANGUAGE CXX)
   target_link_libraries(service --whole-archive config_json --no-whole-archive)
   set(PLUGINS ${PLUGINS} autoconf)
endif()

#
# DRIVERS / PLUGINS - support for parent cmake list specification
#

# Add default stdout driver if option is ON
if (default_stdout)
  set(STDOUT ${STDOUT} default_stdout)
endif()

# Add extra drivers defined from command line
set(DRIVERS ${DRIVERS} ${EXTRA_DRIVERS})
if(DRIVERS)
  list(REMOVE_DUPLICATES DRIVERS) # Remove duplicate drivers
endif()
# Add extra plugins defined from command line
set(PLUGINS ${PLUGINS} ${EXTRA_PLUGINS})
if(PLUGINS)
  list(REMOVE_DUPLICATES PLUGINS) # Remove duplicate plugins
endif()


#
# NACL.TXT
#

if (EXISTS ${CMAKE_SOURCE_DIR}/nacl.txt)
  add_custom_command(
     OUTPUT nacl_content.cpp
     COMMAND cat ${CMAKE_SOURCE_DIR}/nacl.txt | python ${INSTALL_LOC}/nacl/NaCl.py ${CMAKE_BINARY_DIR}/nacl_content.cpp
     DEPENDS ${CMAKE_SOURCE_DIR}/nacl.txt
   )
   add_library(nacl_content STATIC nacl_content.cpp)
   set_target_properties(nacl_content PROPERTIES LINKER_LANGUAGE CXX)
   target_link_libraries(service --whole-archive nacl_content --no-whole-archive)
   set(PLUGINS ${PLUGINS} nacl)
endif()


# Function:
# Add plugin / driver as library, set link options
function(configure_plugin type plugin_name path)
  add_library(${type}_${plugin_name} STATIC IMPORTED)
  set_target_properties(${type}_${plugin_name} PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(${type}_${plugin_name} PROPERTIES IMPORTED_LOCATION ${path})
  target_link_libraries(service --whole-archive ${type}_${plugin_name} --no-whole-archive)
endfunction()

# Function:
# Configure plugins / drivers in a given list provided by e.g. parent script
function(enable_plugins plugin_list search_loc)

  if (NOT ${plugin_list})
    return()
  endif()

  get_filename_component(type ${search_loc} NAME_WE)
  message(STATUS "Looking for ${type} in ${search_loc}")
  foreach(plugin_name ${${plugin_list}})
    unset(path_found CACHE)
    find_library(path_found ${plugin_name} PATHS ${search_loc} NO_DEFAULT_PATH)
    if (NOT path_found)
      message(FATAL_ERROR "Couldn't find " ${type} ":" ${plugin_name})
    else()
      message(STATUS "\t* Found " ${plugin_name})
    endif()
    configure_plugin(${type} ${plugin_name} ${path_found})
  endforeach()
endfunction()

# Function:
# Adds driver / plugin configure option, enables if option is ON
function(plugin_config_option type plugin_list)
  foreach(FILENAME ${${plugin_list}})
    get_filename_component(OPTNAME ${FILENAME} NAME_WE)
    option(${OPTNAME} "Add ${OPTNAME} ${type}" OFF)
    if (${OPTNAME})
      message(STATUS "Enabling ${type} ${OPTNAME}")
      configure_plugin(${type} ${OPTNAME} ${FILENAME})
    endif()
  endforeach()
endfunction()

# Location of installed drivers / plugins
set(STDOUT_LOC ${INSTALL_LOC}/${ARCH}/drivers/stdout)
set(DRIVER_LOC ${INSTALL_LOC}/${ARCH}/drivers)
set(PLUGIN_LOC ${INSTALL_LOC}/${ARCH}/plugins)

# Enable DRIVERS which may be specified by parent cmake list
enable_plugins(STDOUT  ${STDOUT_LOC})
enable_plugins(DRIVERS ${DRIVER_LOC})
enable_plugins(PLUGINS ${PLUGIN_LOC})

# Global lists of installed Drivers / Plugins
file(GLOB STDOUT_LIST "${STDOUT_LOC}/*.a")
file(GLOB DRIVER_LIST "${DRIVER_LOC}/*.a")
file(GLOB PLUGIN_LIST "${PLUGIN_LOC}/*.a")

# Set configure option for each installed driver
plugin_config_option(stdout STDOUT_LIST)
plugin_config_option(driver DRIVER_LIST)
plugin_config_option(plugin PLUGIN_LIST)

# Simple way to build subdirectories before service
foreach(DEP ${DEPENDENCIES})
  get_filename_component(DIR_PATH "${DEP}" DIRECTORY BASE_DIR "${CMAKE_SOURCE_DIR}")
  get_filename_component(DEP_NAME "${DEP}" NAME BASE_DIR "${CMAKE_SOURCE_DIR}")
  #get_filename_component(BIN_PATH "${DEP}" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  add_subdirectory(${DIR_PATH})
  add_dependencies(service ${DEP_NAME})
endforeach()

# Add extra libraries defined from command line
set(LIBRARIES ${LIBRARIES} ${EXTRA_LIBRARIES})
if(LIBRARIES)
  list(REMOVE_DUPLICATES LIBRARIES) # Remove duplicate libraries
endif()

# add all extra libs
set(LIBR_CMAKE_NAMES)
foreach(LIBR ${LIBRARIES})
  # if relative path but not local, use includeos lib.
  if(NOT IS_ABSOLUTE ${LIBR} AND NOT EXISTS ${LIBR})
    set(OS_LIB "$ENV{INCLUDEOS_PREFIX}/includeos/${ARCH}/lib/${LIBR}")
    if(EXISTS ${OS_LIB})
      message(STATUS "Cannot find local ${LIBR}; using ${OS_LIB} instead")
      set(LIBR ${OS_LIB})
    endif()
  endif()
  # add as whole archive to allow strong symbols
  list(APPEND LIBR_CMAKE_NAMES "--whole-archive ${LIBR} --no-whole-archive")
endforeach()


# includes
include_directories(${LOCAL_INCLUDES})
include_directories(${INSTALL_LOC}/${ARCH}/include/libcxx)
include_directories(${INSTALL_LOC}/${ARCH}/include/musl)
include_directories(${INSTALL_LOC}/${ARCH}/include/libunwind)
if ("${PLATFORM}" STREQUAL "x86_solo5")
  include_directories(${INSTALL_LOC}/${ARCH}/include/solo5)
endif()

include_directories(${INSTALL_LOC}/${ARCH}/include)
include_directories(${INSTALL_LOC}/api)
include_directories(${INSTALL_LOC}/include)
include_directories($ENV{INCLUDEOS_PREFIX}/include)


# linker stuff
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS) # this removed -rdynamic from linker output
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> -o <TARGET> <LINK_FLAGS> <OBJECTS> <LINK_LIBRARIES>")

set(BUILD_SHARED_LIBRARIES OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

set(LD_STRIP)
if (NOT debug)
  set(LD_STRIP "--strip-debug")
endif()

set(ELF ${ARCH})
if (${ELF} STREQUAL "i686")
  set(ELF "i386")
endif()

set(PRE_BSS_SIZE  "--defsym PRE_BSS_AREA=0x0")
if ("${PLATFORM}" STREQUAL "x86_solo5")
  # pre-BSS memory hole for uKVM global variables
  set(PRE_BSS_SIZE  "--defsym PRE_BSS_AREA=0x200000")
endif()

set(LDFLAGS "-nostdlib -melf_${ELF} --eh-frame-hdr ${LD_STRIP} --script=${INSTALL_LOC}/${ARCH}/linker.ld ${PRE_BSS_SIZE}")

set_target_properties(service PROPERTIES LINK_FLAGS "${LDFLAGS}")

set(CRTN "${INSTALL_LOC}/${ARCH}/lib/crtn.o")
set(CRTI "${INSTALL_LOC}/${ARCH}/lib/crti.o")

target_link_libraries(service ${CRTI})
target_link_libraries(service ${CRT1})

add_library(libos STATIC IMPORTED)
set_target_properties(libos PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libos PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/lib/libos.a)

add_library(libarch STATIC IMPORTED)
set_target_properties(libarch PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libarch PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/lib/libarch.a)

add_library(libplatform STATIC IMPORTED)
set_target_properties(libplatform PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libplatform PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/platform/lib${PLATFORM}.a)

add_library(libbotan STATIC IMPORTED)
set_target_properties(libbotan PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libbotan PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/lib/libbotan-2.a)

if(${ARCH} STREQUAL "x86_64")
  add_library(libssl STATIC IMPORTED)
  set_target_properties(libssl PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(libssl PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/lib/libssl.a)

  add_library(libcrypto STATIC IMPORTED)
  set_target_properties(libcrypto PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(libcrypto PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/lib/libcrypto.a)
  set(OPENSSL_LIBS libssl libcrypto)

  include_directories(${INSTALL_LOC}/${ARCH}/include)
endif()

add_library(libosdeps STATIC IMPORTED)
set_target_properties(libosdeps PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libosdeps PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/lib/libosdeps.a)

add_library(musl_syscalls STATIC IMPORTED)
set_target_properties(musl_syscalls PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(musl_syscalls PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/lib/libmusl_syscalls.a)

add_library(libcxx STATIC IMPORTED)
add_library(cxxabi STATIC IMPORTED)
add_library(libunwind STATIC IMPORTED)

set_target_properties(libcxx PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libcxx PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/lib/libc++.a)
set_target_properties(cxxabi PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(cxxabi PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/lib/libc++abi.a)
set_target_properties(libunwind PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(libunwind PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/lib/libunwind.a)

add_library(libc STATIC IMPORTED)
set_target_properties(libc PROPERTIES LINKER_LANGUAGE C)
set_target_properties(libc PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/lib/libc.a)

add_library(libpthread STATIC IMPORTED)
set_target_properties(libpthread PROPERTIES LINKER_LANGUAGE C)
set_target_properties(libpthread PROPERTIES IMPORTED_LOCATION "${INSTALL_LOC}/${ARCH}/lib/libpthread.a")

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
  set(COMPILER_RT_FILE "${INSTALL_LOC}/${ARCH}/lib/libcompiler.a")
endif()

add_library(libgcc STATIC IMPORTED)
set_target_properties(libgcc PROPERTIES LINKER_LANGUAGE C)
set_target_properties(libgcc PROPERTIES IMPORTED_LOCATION "${COMPILER_RT_FILE}")

if ("${PLATFORM}" STREQUAL "x86_solo5")
  add_library(solo5 STATIC IMPORTED)
  set_target_properties(solo5 PROPERTIES LINKER_LANGUAGE C)
  set_target_properties(solo5 PROPERTIES IMPORTED_LOCATION ${INSTALL_LOC}/${ARCH}/lib/solo5.o)
endif()

# Depending on the output of this command will make it always run. Like magic.
add_custom_command(
    OUTPUT fake_news
    COMMAND cmake -E echo)

# add memdisk
function(add_memdisk DISK)
  get_filename_component(DISK_RELPATH "${DISK}"
                         REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")
  add_custom_command(
    OUTPUT  memdisk.o
    COMMAND python ${INSTALL_LOC}/memdisk/memdisk.py --file memdisk.asm ${DISK_RELPATH}
    COMMAND nasm -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} memdisk.asm -o memdisk.o
    DEPENDS ${DISK_RELPATH}
  )
  add_library(memdisk STATIC memdisk.o)
  set_target_properties(memdisk PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(service --whole-archive memdisk --no-whole-archive)
endfunction()

# automatically build memdisk from folder
function(build_memdisk FOLD)
  get_filename_component(REL_PATH "${FOLD}" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")
  add_custom_command(
      OUTPUT  memdisk.fat
      COMMAND ${INSTALL_LOC}/bin/diskbuilder -o memdisk.fat ${REL_PATH}
      DEPENDS fake_news
      )
  add_custom_target(diskbuilder DEPENDS memdisk.fat)
  add_dependencies(service diskbuilder)
  add_memdisk("${CMAKE_BINARY_DIR}/memdisk.fat")
endfunction()

# build memdisk if defined
if(MEMDISK)
  message(STATUS "Memdisk folder set: " ${MEMDISK})
  build_memdisk(${MEMDISK})
endif()

# call build_memdisk only if MEMDISK is not defined from command line
function(diskbuilder FOLD)
  if(NOT MEMDISK)
    build_memdisk(${FOLD})
  endif()
endfunction()

function(install_certificates FOLD)
  get_filename_component(REL_PATH "${FOLD}" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")
  message(STATUS "Install certificate bundle at ${FOLD}")
  file(COPY ${INSTALL_LOC}/cert_bundle/ DESTINATION ${REL_PATH})
endfunction()

if(CERTS)
  message(STATUS "Certs folder set: " ${CERTS})
  install_certificates(${CERTS})
endif()

if(TARFILE)
  get_filename_component(TAR_RELPATH "${TARFILE}"
                         REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")

  if(CREATE_TAR)
    get_filename_component(TAR_BASE_NAME "${CREATE_TAR}" NAME)
    add_custom_command(
      OUTPUT tarfile.o
      COMMAND tar cf ${TAR_RELPATH} -C ${CMAKE_SOURCE_DIR} ${TAR_BASE_NAME}
      COMMAND cp ${TAR_RELPATH} input.bin
      COMMAND ${CMAKE_OBJCOPY} -I binary -O ${OBJCOPY_TARGET} -B i386 input.bin tarfile.o
      COMMAND rm input.bin
    )
  elseif(CREATE_TAR_GZ)
    get_filename_component(TAR_BASE_NAME "${CREATE_TAR_GZ}" NAME)
    add_custom_command(
      OUTPUT tarfile.o
      COMMAND tar czf ${TAR_RELPATH} -C ${CMAKE_SOURCE_DIR} ${TAR_BASE_NAME}
      COMMAND cp ${TAR_RELPATH} input.bin
      COMMAND ${CMAKE_OBJCOPY} -I binary -O ${OBJCOPY_TARGET} -B i386 input.bin tarfile.o
      COMMAND rm input.bin
    )
  else(true)
    add_custom_command(
      OUTPUT tarfile.o
      COMMAND cp ${TAR_RELPATH} input.bin
      COMMAND ${CMAKE_OBJCOPY} -I binary -O ${OBJCOPY_TARGET} -B i386 input.bin tarfile.o
      COMMAND rm input.bin
    )
  endif(CREATE_TAR)

  add_library(tarfile STATIC tarfile.o)
  set_target_properties(tarfile PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(service --whole-archive tarfile --no-whole-archive)
endif(TARFILE)

if ("${PLATFORM}" STREQUAL "x86_solo5")
  target_link_libraries(service solo5)
endif()

# all the OS and C/C++ libraries + crt end
target_link_libraries(service
  libos
  libplatform
  libarch

  ${LIBR_CMAKE_NAMES}
  libos
  libbotan
  ${OPENSSL_LIBS}
  libosdeps

  libplatform
  libarch

  musl_syscalls
  libos
  libcxx
  cxxabi
  libunwind
  libpthread
  libc

  musl_syscalls
  libos
  libc
  libgcc
  ${CRTN}
  )
# write binary location to known file
file(WRITE ${CMAKE_BINARY_DIR}/binary.txt ${BINARY})

# old behavior: remove all symbols after elfsym
if (NOT debug)
  set(STRIP_LV ${CMAKE_STRIP} --strip-debug ${BINARY})
else()
  set(STRIP_LV true)
endif()

add_custom_target(
  pruned_elf_symbols ALL
  COMMAND ${INSTALL_LOC}/bin/elf_syms ${BINARY}
  COMMAND ${CMAKE_OBJCOPY} --update-section .elf_symbols=_elf_symbols.bin ${BINARY} ${BINARY}
  COMMAND ${STRIP_LV}
  DEPENDS service
  )

# create bare metal .img: make legacy_bootloader
add_custom_target(
  legacy_bootloader
  COMMAND ${INSTALL_LOC}/bin/vmbuild ${BINARY} ${INSTALL_LOC}/${ARCH}/boot/bootloader
  DEPENDS service
)

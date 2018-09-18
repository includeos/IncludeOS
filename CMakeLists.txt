cmake_minimum_required(VERSION 3.1.0)

set(INCLUDEOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

# Target CPU Architecture
if(DEFINED ENV{ARCH})
  set(ARCH "$ENV{ARCH}" CACHE STRING "Architecture")
else()
  set(ARCH "x86_64" CACHE STRING "Architecture (default)")
endif()

message(STATUS "Target CPU ${ARCH}")

set(TRIPLE "${ARCH}-pc-linux-elf")
set(CMAKE_CXX_COMPILER_TARGET ${TRIPLE})
set(CMAKE_C_COMPILER_TARGET ${TRIPLE})

message(STATUS "Target triple ${TRIPLE}")

option(WITH_SOLO5 "Install with solo5 support" ON)

set(CMAKE_TOOLCHAIN_FILE ${INCLUDEOS_ROOT}/cmake/elf-toolchain.cmake)

project (includeos C CXX)

set(INC ${CMAKE_INSTALL_PREFIX}/includeos/include)
set(LIB ${CMAKE_INSTALL_PREFIX}/includeos/lib)
set(BIN ${CMAKE_INSTALL_PREFIX}/includeos/bin)
set(SCRIPTS ${CMAKE_INSTALL_PREFIX}/includeos/scripts)

# C++ version
set(CPP_VERSION c++17)

# create OS version string from git describe (used in CXX flags)
execute_process(COMMAND git describe --tags --dirty
	        WORKING_DIRECTORY ${INCLUDEOS_ROOT}
	        OUTPUT_VARIABLE OS_VERSION)
string(STRIP ${OS_VERSION} OS_VERSION)

option(cpu_feat_vanilla "Restrict use of CPU features to vanilla" ON)
if(cpu_feat_vanilla)
  include("cmake/vanilla.cmake")
  set(DEFAULT_SETTINGS_CMAKE "vanilla.cmake") # for service cmake
  set(DEFAULT_VM "vm.vanilla.json") # vmrunner
else()
  include("cmake/cpu_feat.cmake")
  set(DEFAULT_SETTINGS_CMAKE "cpu_feat.cmake") # for service cmake
  set(DEFAULT_VM "vm.cpu_feat.json") # vmrunner
endif(cpu_feat_vanilla)

option(smp "Compile with SMP (multiprocessing)" OFF)

option(silent "Disable most output during OS boot" OFF)

option (undefined_san "Enable undefined-behavior sanitizer" OFF)
option (thin_lto "Enable the Thin LTO plugin" OFF)
option (full_lto "Enable full LTO (compatibility)" OFF)

# create random hex string as stack protector canary
string(RANDOM LENGTH 16 ALPHABET 0123456789ABCDEF STACK_PROTECTOR_VALUE)

set(CAPABS "${CAPABS} -g -fstack-protector-strong -D_STACK_GUARD_VALUE_=0x${STACK_PROTECTOR_VALUE}")

# Various global defines
# * NO_DEBUG disables output from the debug macro
# * OS_TERMINATE_ON_CONTRACT_VIOLATION provides classic assert-like output from Expects / Ensures
set(CAPABS "${CAPABS} -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_LIBCPP_HAS_MUSL_LIBC -D_GNU_SOURCE -D__includeos__")
set(WARNS "-Wall -Wextra") # -Werror

# configure options
option(debug "Build with no optimizations" OFF)
option(minimal "Build for minimal size" OFF)
option(stripped "reduce size" OFF)

function(init_submodule MOD)
  message(STATUS "Init git submodule: " ${MOD})
  execute_process(COMMAND git submodule update --init ${MOD} WORKING_DIRECTORY ${INCLUDEOS_ROOT})
endfunction()

# Init submodules
init_submodule(mod/GSL)
init_submodule(mod/http-parser)
init_submodule(mod/uzlib)
init_submodule(mod/rapidjson)
init_submodule(NaCl)

# set optimization level
set(OPTIMIZE "-O2")

if(debug OR debug-info OR debug-all)
	set(OPTIMIZE "-O0")
endif(debug OR debug-info OR debug-all)

if(minimal)
	set(OPTIMIZE "-Os")
endif(minimal)

if(silent)
	set(CAPABS "${CAPABS} -DNO-INFO=1")
endif(silent)

# Append optimization level
set(CAPABS "${CAPABS} ${OPTIMIZE}")

# Append sanitizers
if (undefined_san)
  set(CAPABS "${CAPABS} -fsanitize=undefined -fno-sanitize=vptr")
endif()
if (thin_lto)
  set(CAPABS "${CAPABS} -flto=thin")
elseif(full_lto)
  set(CAPABS "${CAPABS} -flto=full")
endif()

# object format needs to be set BEFORE enabling ASM
# see: https://cmake.org/Bug/bug_relationship_graph.php?bug_id=13166
if ("${ARCH}" STREQUAL "i686")
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf")
  set(OBJCOPY_TARGET "elf32-i386")
  set(CAPABS "${CAPABS} -m32")
else()
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf64")
  set(OBJCOPY_TARGET "elf64-x86-64")
  set(CAPABS "${CAPABS} -m64")
endif()

enable_language(ASM_NASM)

# initialize C and C++ compiler flags
if (CMAKE_COMPILER_IS_GNUCC)
  # gcc/g++ settings
  set(CMAKE_CXX_FLAGS "${CAPABS} -std=${CPP_VERSION} ${WARNS} -Wno-frame-address -nostdlib -fno-omit-frame-pointer -c")
  set(CMAKE_C_FLAGS " ${CAPABS} ${WARNS} -nostdlib -fno-omit-frame-pointer -c")
else()
  # these kinda work with llvm
  set(CMAKE_CXX_FLAGS "${CAPABS} -std=${CPP_VERSION} ${WARNS} -nostdlib -nostdlibinc -fno-omit-frame-pointer -c")
  set(CMAKE_C_FLAGS "${CAPABS} ${WARNS} -nostdlib -nostdlibinc -fno-omit-frame-pointer -c")
endif()

# either download or cross-compile needed libraries
#option(from_bundle "Download and use pre-compiled libraries for cross-comilation" ON)
set(BUNDLE_LOC "" CACHE STRING "Local path of bundle with pre-compile libraries")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cross_compiled_libraries.cmake)

# Botan Crypto & TLS
# Note: Include order matters!
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/botan.cmake)
# OpenSSL libssl/libcrypto
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/openssl.cmake)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/nacl.cmake)

#
# Subprojects
#
add_subdirectory(mod)
add_subdirectory(src)

#
# External projects
#
option(vmbuild "Build and install vmbuild and elf_syms" ON)
if(vmbuild)
  # Install vmbuilder as an external project
  ExternalProject_Add(vmbuild
    PREFIX vmbuild # Build where
    BUILD_ALWAYS 1
    SOURCE_DIR ${INCLUDEOS_ROOT}/vmbuild # Where is project located
    BINARY_DIR ${INCLUDEOS_ROOT}/vmbuild/build
    INSTALL_DIR ${BIN} # Where to install
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
     -DINCLUDE_PATH=${CMAKE_INSTALL_PREFIX} # Pass installation folder
    DEPENDS PrecompiledLibraries
    )
endif(vmbuild)

option(diskbuilder "Build and install memdisk helper tool" ON)
if(diskbuilder)
  ExternalProject_Add(diskbuilder
    BUILD_ALWAYS 1
    SOURCE_DIR ${INCLUDEOS_ROOT}/diskimagebuild
    BINARY_DIR ${INCLUDEOS_ROOT}/diskimagebuild/build
    INSTALL_DIR ${BIN}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  )
endif(diskbuilder)

option(examples "Build example unikernels in /examples" OFF)
if(examples)
  set(libprotobuf ON) # dependent
  add_subdirectory(examples)
endif(examples)

option(tests "Build unit tests in /test and install lest test framework" OFF)
option(lest "Install lest unittest headers" OFF)

if (lest OR tests)
  init_submodule(test/lest)
  install(DIRECTORY test/lest/include/lest DESTINATION ${CMAKE_INSTALL_PREFIX}/includeos/include)
endif()

if(tests)
  enable_testing()
  ExternalProject_Add(unittests
    PREFIX unittests
    SOURCE_DIR ${INCLUDEOS_ROOT}/test
    BINARY_DIR unittests
    CMAKE_ARGS -DINCLUDEOS_ROOT=${INCLUDEOS_ROOT} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    )
  #add_subdirectory(test)
endif(tests)

#
# Libraries
#
option(libmana "Build and install mana web application framework library" ON)
if(libmana)
  add_subdirectory(lib/mana)
endif(libmana)

option(libuplink "Build and install uplink" ON)
if(libuplink)
  set(libliveupdate ON) # dependent
  add_subdirectory(lib/uplink)
endif(libuplink)

option(libmender "Build and install mender client" ON)
if(libmender)
  set(libliveupdate ON) # dependent
  add_subdirectory(lib/mender)
endif(libmender)

option(libliveupdate "Build and install LiveUpdate" ON)
if(libliveupdate)
  add_subdirectory(lib/LiveUpdate)
endif(libliveupdate)

option(libmicroLB "Build and install microLB" ON)
if(libmicroLB)
  add_subdirectory(lib/microLB)
endif()


option(libprotobuf "Build and install Google's protobuf runtime library" ON)
if(libprotobuf)
  init_submodule(lib/protobuf)
  include(${INCLUDEOS_ROOT}/cmake/protobuf.cmake)
endif(libprotobuf)

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

# Install cmake files
install(FILES cmake/pre.service.cmake DESTINATION includeos)
install(FILES cmake/post.service.cmake DESTINATION includeos)
install(FILES cmake/linux.service.cmake DESTINATION includeos)
install(FILES cmake/library.cmake DESTINATION includeos)
install(FILES cmake/${DEFAULT_SETTINGS_CMAKE} DESTINATION includeos RENAME settings.cmake) # cpu_feat_vanilla opt

# Install vmrunner
install(DIRECTORY vmrunner DESTINATION includeos)
install(FILES vmrunner/${DEFAULT_VM} DESTINATION includeos/vmrunner/ RENAME vm.default.json) # cpu_feat_vanilla opt

# Install toolchain
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/elf-toolchain.cmake DESTINATION includeos)

# Install seed
install(DIRECTORY seed/ DESTINATION includeos/seed)

# Install executable scripts
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/etc/boot DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/etc/linux/lxp-run DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/etc/linux/lxp-callgraph DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/etc/linux/lxp-debug DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/etc/linux/lxp-gprof DESTINATION bin)

# Install scripts
install(PROGRAMS
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/create_bridge.sh
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/create_memdisk.sh
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/grubify.sh
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/qemu-ifup
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/qemu-ifdown
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/qemu_cmd.sh
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/ukvm-ifup.sh
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/run.sh
  DESTINATION includeos/scripts)

install(DIRECTORY api/ DESTINATION includeos/api)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mod/GSL/gsl DESTINATION includeos/include)
install(DIRECTORY mod/rapidjson/include/rapidjson DESTINATION includeos/include)

set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_PACKAGE_VERSION ${OS_VERSION})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ingve")
include(CPack)

cmake_minimum_required(VERSION 2.8.9)

set(INCLUDEOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

set(CMAKE_TOOLCHAIN_FILE ${INCLUDEOS_ROOT}/cmake/i686-elf-toolchain.cmake)

project (includeos)

set(LIB ${CMAKE_INSTALL_PREFIX}/includeos/lib)
set(BIN ${CMAKE_INSTALL_PREFIX}/includeos/bin)
set(SCRIPTS ${CMAKE_INSTALL_PREFIX}/includeos/scripts)

# test compiler
if(CMAKE_COMPILER_IS_GNUCC)
	# currently gcc is not supported due to problems cross-compiling a unikernel
	# (i.e., building a 32bit unikernel (only supported for now) on a 64bit system)
	message(FATAL_ERROR "Building IncludeOS with gcc is not currently supported. Please clean-up build directory and configure for clang through CC and CXX environmental variables.")
endif(CMAKE_COMPILER_IS_GNUCC)

# create OS version string from git describe (used in CXX flags)
execute_process(COMMAND git describe --dirty
	        WORKING_DIRECTORY ${INCLUDEOS_ROOT}
	        OUTPUT_VARIABLE OS_VERSION)
string(STRIP ${OS_VERSION} OS_VERSION)

# create random hex string as stack protector canary
string(RANDOM LENGTH 8 ALPHABET 0123456789ABCDEF STACK_PROTECTOR_VALUE)

set(CAPABS "-msse3 -fstack-protector-strong -D_STACK_GUARD_VALUE_=0x${STACK_PROTECTOR_VALUE} ")

# Various global defines
# * NO_DEBUG disables output from the debug macro
# * OS_TERMINATE_ON_CONTRACT_VIOLATION provides classic assert-like output from Expects / Ensures
# * _GNU_SOURCE enables POSIX-extensions in newlib, such as strnlen. ("everything newlib has", ref. cdefs.h)
set(CAPABS "${CAPABS} -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE")

set(WARNS "-Wall -Wextra") #-pedantic

# configure options
option(debug "Build with debugging symbols (OBS: Dramatically increases binary size)" OFF)
option(debug-info "Build like \"all\" but with debugging output (i.e. the 'debug'-macro) enabled" OFF)
option(debug-all "Build with debugging symbols + debugging output, i.e. \"debug\" + \"debug-info\"" OFF)
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

# set optimization level
set(OPTIMIZE "-O2")

if(debug OR debug-info OR debug-all)
	set(OTPIMIZE "-O0")
endif(debug OR debug-info OR debug-all)

if(minimal)
	set(OPTIMIZE "-Os")
endif(minimal)

# Set debug options
if(debug OR debug-all)
	set(CAPABS "${CAPABS} -ggdb3 -DGSL_THROW_ON_CONTRACT_VIOLATION")
endif(debug OR debug-all)

if(debug-info OR debug-all)
	set(CAPABS "${CAPABS} -UNO_DEBUG")
endif(debug-info OR debug-all)

option(silent OFF)
if(silent)
	set(CAPABS "${CAPABS} -DNO-INFO=1")
endif(silent)

# Append optimization level
set(CAPABS "${CAPABS} ${OPTIMIZE}")

# these kinda work with llvm
set(CMAKE_CXX_FLAGS "-target i686 -MMD ${CAPABS} ${WARNS} -nostdlib -nostdlibinc -c -m32 -std=c++14 -D_LIBCPP_HAS_NO_THREADS=1 -DOS_VERSION=\\\"${OS_VERSION}\\\"")
set(CMAKE_C_FLAGS "-target i686 -MMD ${CAPABS} ${WARNS} -nostdlib -nostdlibinc -c -m32 -DOS_VERSION=\"\"${OS_VERSION}\"\"")

# either download or cross-compile needed libraries
option(from_bundle "Download and use pre-compiled libraries for cross-comilation" ON)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cross_compiled_libraries.txt)

# Botan Crypto & TLS
# Note: Include order matters!
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/botan.cmake)

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
    SOURCE_DIR ${INCLUDEOS_ROOT}/vmbuild # Where is project located
    BINARY_DIR ${INCLUDEOS_ROOT}/vmbuild/build
    INSTALL_DIR ${BIN} # Where to install
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> # Pass installation folder
    )
endif(vmbuild)

option(diskbuilder "Build and install memdisk helper tool" ON)
if(diskbuilder)
  ExternalProject_Add(diskbuilder
    SOURCE_DIR ${INCLUDEOS_ROOT}/diskimagebuild
    BINARY_DIR ${INCLUDEOS_ROOT}/diskimagebuild/build
    INSTALL_DIR ${BIN}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  )
endif(diskbuilder)

option(examples "Build example unikernels in /examples" OFF)
if(examples)
	add_subdirectory(examples)
endif(examples)

install(DIRECTORY api/ DESTINATION includeos/api)

option(tests "Build unit tests in /test and install lest test framework" OFF)
if(tests)
  init_submodule(test/lest)
  enable_testing()
  ExternalProject_Add(unittests
    PREFIX unittests
    SOURCE_DIR ${INCLUDEOS_ROOT}/test
    CMAKE_ARGS -DINCLUDEOS_ROOT=${INCLUDEOS_ROOT} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    )
  #add_subdirectory(test)
endif(tests)

option(libmana "Build and install mana web application framework library" ON)
if(libmana)
  set(rapidjson ON) # Dependent on rapidjson
  add_subdirectory(lib/mana)
endif(libmana)

option(rapidjson "Download and install rapidjson submodule" ON)
if(rapidjson)
  init_submodule(mod/rapidjson)
  install(DIRECTORY mod/rapidjson/include/rapidjson DESTINATION includeos/include)
endif(rapidjson)


#
# Installation
#
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/etc/service.cmake DESTINATION includeos)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/etc/library.cmake DESTINATION includeos)
install(DIRECTORY vmrunner DESTINATION includeos/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/i686-elf-toolchain.cmake DESTINATION includeos)
install(DIRECTORY seed/ DESTINATION includeos/seed)

# Install boot util
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/etc/boot DESTINATION bin)

# Install scripts
install(PROGRAMS
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/create_bridge.sh
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/create_memdisk.sh
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/grubify.sh
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/qemu-ifup
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/qemu_cmd.sh
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/scripts/run.sh
  DESTINATION includeos/scripts)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mod/GSL/gsl DESTINATION include)

set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_PACKAGE_VERSION ${OS_VERSION})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ingve")
include(CPack)

cmake_minimum_required(VERSION 3.1.0)

project (includeos C CXX)

set(INCLUDEOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag(-std=c++17 HAVE_FLAG_STD_CXX17)
if(NOT HAVE_FLAG_STD_CXX17)
 message(FATAL_ERROR "The provided compiler: " ${CMAKE_CXX_COMPILER} "\n does not support c++17 standard please make sure your CC and CXX points to a compiler that supports c++17")
endif()


if ((CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT))
  if (DEFINED ENV{INCLUDEOS_PREFIX})
    set(CMAKE_INSTALL_PREFIX $ENV{INCLUDEOS_PREFIX} CACHE PATH "..." FORCE)
  else()
    set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/includeos)
    message(WARNING "CMAKE_INSTALL_PREFIX is default ${CMAKE_INSTALL_PREFIX} did you forget to set INCLUDEOS_PREFIX")
  endif()
endif()

#TODO get this from profile ?
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

if (NOT APPLE)
  option(WITH_SOLO5 "Install with solo5 support" ON)
endif()

set(CMAKE_TOOLCHAIN_FILE ${INCLUDEOS_ROOT}/cmake/elf-toolchain.cmake)

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
set(CAPABS "${CAPABS} -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE -D__includeos__")
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
init_submodule(NaCl)


#TODO get all these from profile ?
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
  set(CMAKE_CXX_FLAGS "${CAPABS}  -std=${CPP_VERSION} ${WARNS} -Wno-frame-address -nostdlib -fno-omit-frame-pointer -c")
  set(CMAKE_C_FLAGS " ${CAPABS} ${WARNS}  -nostdlib -fno-omit-frame-pointer -c")
else()
  # these kinda work with llvm
  set(CMAKE_CXX_FLAGS "${CAPABS}  -std=${CPP_VERSION} ${WARNS} -nostdlib -nostdlibinc -fno-omit-frame-pointer -c")
  set(CMAKE_C_FLAGS "${CAPABS} ${WARNS} -nostdlib  -nostdlibinc -fno-omit-frame-pointer -c")
endif()


if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

#TODO fix this
set(CMAKE_BUILD_TYPE Release)

#NB 5.0 is llvm version should be a variable from CMAKE or profile ?
#TODO separate dependencies if build is GCC from if build is clang in
#conanfile.py and add requirement for the tools in the profile!!
#TODO create a NaCL package ?
#TODO create packages for "other" lib

#Sets the includeos default profile to clang-7.0 mergeconflict?
if (NOT DEFINED CONAN_PROFILE)
  SET(CONAN_PROFILE clang-5.0)
endif()

set(CONAN_APPLE False)
IF (APPLE)
  set(CONAN_APPLE True)
ENDIF(APPLE)

#Are we executing cmake from conan or locally
#if locally then pull the deps from conanfile.py
#if buiding from conan expect the conanbuildinfo.cmake to already be present
if(CONAN_EXPORTED) # in conan local cache
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
else() # in user space
  include(${CMAKE_BINARY_DIR}/conan.cmake)

  conan_check(VERSION 1.8.4 REQUIRED)
  #TODO check if we can check if remote is added and give a warning if its
  #missing ?
  #conan_add_remote(NAME includeos INDEX 1
  #          URL https://api.bintray.com/conan/bincrafters/public-conan)
  #include(conan.cmake)
  # Make sure to use conanfile.py to define dependencies, to stay consistent
  conan_cmake_run(
    CONANFILE conan/includeos/conanfile.py
    PROFILE ${CONAN_PROFILE}
		OPTIONS apple=${CONAN_APPLE} solo5=${CONAN_SOLO5}
    BASIC_SETUP
    BUILD missing
  )
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/nacl.cmake)

#
# Subprojects
#

add_subdirectory(src)

#
# External projects
#

option(diskbuilder "Build and install memdisk helper tool" ON)
if(diskbuilder)
  ExternalProject_Add(diskbuilder
    SOURCE_DIR ${INCLUDEOS_ROOT}/diskimagebuild
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/diskimagebuild/build
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
#TODO verify the asumption that all these work once all the proper includes are in order!!

#option(libmana "Build and install mana web application framework library" ON)
#if(libmana)
#  add_subdirectory(lib/mana)
#endif(libmana)

#option(libuplink "Build and install uplink" ON)
#if(libuplink)
#  set(libliveupdate ON) # dependent
#  add_subdirectory(lib/uplink)
#endif(libuplink)

#option(libmender "Build and install mender client" ON)
#if(libmender)
#  set(libliveupdate ON) # dependent
#  add_subdirectory(lib/mender)
#endif(libmender)

#option(libliveupdate "Build and install LiveUpdate" ON)
#if(libliveupdate)
#  add_subdirectory(lib/LiveUpdate)
#endif(libliveupdate)
#
#option(libmicroLB "Build and install microLB" ON)
#if(libmicroLB)
#  add_subdirectory(lib/microLB)
#endif()

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

# Install cmake files
install(FILES cmake/pre.service.cmake DESTINATION cmake)
install(FILES cmake/post.service.cmake DESTINATION cmake)
install(FILES cmake/linux.service.cmake DESTINATION cmake)
install(FILES cmake/library.cmake DESTINATION cmake)
install(FILES cmake/${DEFAULT_SETTINGS_CMAKE} DESTINATION cmake RENAME settings.cmake) # cpu_feat_vanilla opt

# Install vmrunner
install(DIRECTORY vmrunner DESTINATION tools)
install(FILES vmrunner/${DEFAULT_VM} DESTINATION tools/vmrunner/ RENAME vm.default.json) # cpu_feat_vanilla opt

# TODO ? move to "linux cmake"
# Install toolchain
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/elf-toolchain.cmake DESTINATION cmake)

# Install seed
install(DIRECTORY seed/ DESTINATION seed)

# Install executable scripts
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/etc/boot DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/etc/linux/lxp-run DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/etc/linux/lxp-callgraph DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/etc/linux/lxp-debug DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/etc/linux/lxp-gprof DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/etc/linux/lxp-pgo DESTINATION bin)

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
  DESTINATION scripts)

if(NOT CONAN_EXPORTED)
  foreach(MODULE ${CONAN_CMAKE_MODULE_PATH})
    #message("INPUT ${MODULE}")
    string(REGEX MATCH ".*conan[\\\/]data[\\\/](.[a-zA-Z0-9_.-]*[^\\\/])[\\\/](.[a-zA-Z0-9_.-]*[^\\\/])[\\\/](.[a-zA-Z0-9_.-]*[^\\\/])[\\\/](.[a-zA-Z0-9_.-]*[^\\\/])[\\\/]" MOD ${MODULE})
    set(NAME ${CMAKE_MATCH_1})
    set(VERSION ${CMAKE_MATCH_2})
    set(USER ${CMAKE_MATCH_3})
    set(CHANNEL ${CMAKE_MATCH_4})
    install(CODE
    "execute_process(COMMAND conan install ${NAME}/${VERSION}@${USER}/${CHANNEL} -pr ${CONAN_PROFILE} -if ${CMAKE_INSTALL_PREFIX}/${ARCH})"
    )
  endforeach()
endif(NOT CONAN_EXPORTED)

install(DIRECTORY api/ DESTINATION include/os)

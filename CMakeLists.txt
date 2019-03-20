cmake_minimum_required(VERSION 3.1.0)

project (includeos C CXX)

option(WITH_SOLO5 "Build with solo5 support" OFF)

#Sets the includeos default profile to clang-5.0
if (NOT DEFINED CONAN_PROFILE)
  SET(CONANPROFILE "")
else()
  set(CONANPROFILE PROFILE ${CONAN_PROFILE})
endif()

#Are we executing cmake from conan or locally
#if locally then pull the deps from conanfile.py
#if buiding from conan expect the conanbuildinfo.cmake to already be present
if(CONAN_EXPORTED OR EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake) # in conan local cache
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
else() # in user space
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
  endif()
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
     message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
     file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/develop/conan.cmake"
                    "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()
  include(${CMAKE_BINARY_DIR}/conan.cmake)

  conan_check(VERSION 1.8.4 REQUIRED)

  conan_cmake_run(
    CONANFILE conanfile.py
    PROFILE ${CONAN_PROFILE}
    OPTIONS apple=${APPLE} solo5=${WITH_SOLO5} basic=${CORE_OS}
    BASIC_SETUP
    NO_IMPORTS
    ${CONANPROFILE}
  )
endif()

#TODO get from conan ?
# Target CPU Architecture
if (NOT ARCH)
  if (CMAKE_SYSTEM_PROCESSOR)
    set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
  elseif(ENV{ARCH})
    set(ARCH $ENV{ARCH})
  else()
    set(ARCH "x86_64")
  endif()
endif()

message(STATUS "Target CPU ${ARCH}")

#only for clang.. and probably only for apple or when we dont have a cross compiler..
set(TRIPLE "${ARCH}-pc-linux-elf")
set(CMAKE_CXX_COMPILER_TARGET ${TRIPLE})
set(CMAKE_C_COMPILER_TARGET ${TRIPLE})

message(STATUS "Target triple ${TRIPLE}")

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#TODO fix this by using cmake's well defined buld type definitions


include(ExternalProject)
include(CMakeDependentOption)

#if not apple and x86_64 enable with solo5.. this option results in a different arch in conan.. should be handled like that as well
#cmake_dependent_option(WITH_SOLO5 "Install with solo5 support" ON
#"NOT APPLE;${ARCH} STREQUAL x86_64" OFF)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

##extract version header only ONCE avoiding endless rebuild loop ?
FIND_PACKAGE(Git REQUIRED)
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.h.in
"#define OS_VERSION \"@VERSION@\"\n"
)

FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.cmake
"
  FIND_PACKAGE(Git REQUIRED)
  EXECUTE_PROCESS(
     COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
     OUTPUT_VARIABLE VERSION
     OUTPUT_STRIP_TRAILING_WHITESPACE
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
 )
 CONFIGURE_FILE(\${SRC} \${DST} @ONLY)
")

add_custom_target(
      version
      ${CMAKE_COMMAND} -D SRC=${CMAKE_CURRENT_BINARY_DIR}/version.h.in
                       -D DST=${CMAKE_CURRENT_BINARY_DIR}/version.h
                       -P ${CMAKE_CURRENT_BINARY_DIR}/version.cmake
)

include_directories(${CMAKE_BINARY_DIR})

cmake_dependent_option(CORE_OS "Only build the core OS reducing dependencies on botan,openssl etc" OFF
  "NOT PLATFORM STREQUAL x86_nano" ON)

#option(cpu_feat_vanilla "Restrict use of CPU features to vanilla" ON)
#if(cpu_feat_vanilla)
#  include("cmake/vanilla.cmake")
#  set(DEFAULT_SETTINGS_CMAKE "vanilla.cmake") # for service cmake
#  set(DEFAULT_VM "vm.vanilla.json") # vmrunner
#else()
#  include("cmake/cpu_feat.cmake")
#  set(DEFAULT_SETTINGS_CMAKE "cpu_feat.cmake") # for service cmake
#  set(DEFAULT_VM "vm.cpu_feat.json") # vmrunner
#endif(cpu_feat_vanilla)

option(smp "Compile with SMP (multiprocessing)" OFF)

option(silent "Disable most output during OS boot" OFF)

#option (undefined_san "Enable undefined-behavior sanitizer" OFF)
#option (thin_lto "Enable the Thin LTO plugin" OFF)
#option (full_lto "Enable full LTO (compatibility)" OFF)

set(CAPABS "${CAPABS} -g -fstack-protector-strong")

# Various global defines
# * NO_DEBUG disables output from the debug macro
# * OS_TERMINATE_ON_CONTRACT_VIOLATION provides classic assert-like output from Expects / Ensures
set(CAPABS "${CAPABS} -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE -D__includeos__")
set(WARNS "-Wall -Wextra") # -Werror

# configure options
option(debug "Build with no optimizations" OFF)
option(minimal "Build for minimal size" OFF)
option(stripped "reduce size" OFF)

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
if ("${ARCH}" STREQUAL "i686" OR "${ARCH}" STREQUAL "i386" )
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf")
  set(OBJCOPY_TARGET "elf32-i386")
  set(CAPABS "${CAPABS} -m32")
  enable_language(ASM_NASM)
elseif ("${ARCH}" STREQUAL "aarch64")
  #In cmake we trust
  #set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf64")
  #TODO set(OBJCOPY_TARGET "elf64-x86-64")
  #set(CAPABS "${CAPABS} -m64")
else()
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf64")
  set(OBJCOPY_TARGET "elf64-x86-64")
  set(CAPABS "${CAPABS} -m64")
  enable_language(ASM_NASM)
endif()


#TODO these should all come from the conan profile!!!!!!
# initialize C and C++ compiler flags
if (CMAKE_COMPILER_IS_GNUCC)
  # gcc/g++ settings
  set(CMAKE_CXX_FLAGS "${CAPABS} ${WARNS} -Wno-frame-address -nostdlib -fno-omit-frame-pointer -c ${CMAKE_CXX_FLAGS}")
  set(CMAKE_C_FLAGS " ${CAPABS} ${WARNS}  -nostdlib -fno-omit-frame-pointer -c ${CMAKE_C_FLAGS}")
else()
  # these kinda work with llvm
  set(CMAKE_CXX_FLAGS "${CAPABS} ${WARNS} -nostdlib -nostdlibinc -fno-omit-frame-pointer -c ${CMAKE_CXX_FLAGS}")
  set(CMAKE_C_FLAGS "${CAPABS} ${WARNS} -nostdlib  -nostdlibinc -fno-omit-frame-pointer -c ${CMAKE_C_FLAGS}")
endif()

#
# Subprojects
#
add_subdirectory(src)

# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam

# Install cmake files
install(FILES cmake/linux.service.cmake DESTINATION cmake)
install(FILES cmake/library.cmake DESTINATION cmake)
install(FILES cmake/os.cmake DESTINATION cmake)
#install(FILES cmake/${DEFAULT_SETTINGS_CMAKE} DESTINATION cmake RENAME settings.cmake) # cpu_feat_vanilla opt


# TODO ? move to "linux cmake" this is only apple
# Install toolchain
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/elf-toolchain.cmake DESTINATION cmake)


install(DIRECTORY api/ DESTINATION include/os)

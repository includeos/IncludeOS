cmake_minimum_required(VERSION 3.6.0)

#we are only creating libraries for ELF
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (includeos C CXX)

#this can also come from conan
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


option(SMP "Compile with SMP (multiprocessing)" OFF)

#Are we executing cmake from conan or locally
#if locally then pull the deps from conanfile.py
#if buiding from conan expect the conanbuildinfo.cmake to already be present
if(CONAN_EXPORTED OR EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake) # in conan local cache
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${CONAN_SETTINGS_BUILD_TYPE})
  endif()

else() # in user space

  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
  endif()
  #Sets the includeos default profile to clang-5.0
  if (DEFINED CONAN_PROFILE)
    set(CONANPROFILE PROFILE ${CONAN_PROFILE})
  endif()

  #ordering matters we create opts here
  if (SMP)
    set(OPTS OPTIONS smp="True")
  endif()

  if (PLATFORM)
    if (OPTS)
      list(APPEND OPTS platform=${PLATFORM})
    else()
      set(OPTS OPTIONS platform=${PLATFORM})
    endif()
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
    ${OPTS}
    BASIC_SETUP
    ${CONANPROFILE}
  )
endif()

if (NOT PLATFORM)
  set(PLATFORM "default")
endif()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ${CONAN_SETTINGS_BUILD_TYPE})
endif()

#uncreachable W/O conan
# Target CPU Architecture
if (NOT ARCH)
  if (CONAN_SETTINGS_ARCH)
    if ("${CONAN_SETTINGS_ARCH}" STREQUAL "armv8")
      set(ARCH "aarch64")
    elseif("${CONAN_SETTINGS_ARCH}" STREQUAL "x86")
      set(ARCH "i686")
    else()
      set(ARCH ${CONAN_SETTINGS_ARCH})
    endif()
  elseif (CMAKE_SYSTEM_PROCESSOR)
    set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
  elseif(ENV{ARCH})
    set(ARCH $ENV{ARCH})
  else()
    set(ARCH "x86_64")
  endif()
endif()

message(STATUS "Target CPU ${ARCH}")
set(TRIPLE "${ARCH}-pc-linux-elf")
set(CMAKE_CXX_COMPILER_TARGET ${TRIPLE})
set(CMAKE_C_COMPILER_TARGET ${TRIPLE})
message(STATUS "Target triple ${TRIPLE}")


set(CAPABS "${CAPABS} -g -fstack-protector-strong")

# Various global defines
# * NO_DEBUG disables output from the debug macro
# * OS_TERMINATE_ON_CONTRACT_VIOLATION provides classic assert-like output from Expects / Ensures
set(CAPABS "${CAPABS} -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE -D__includeos__")
set(WARNS "-Wall -Wextra") # -Werror

# object format needs to be set BEFORE enabling ASM
# see: https://cmake.org/Bug/bug_relationship_graph.php?bug_id=13166
if ("${ARCH}" STREQUAL "i686" OR "${ARCH}" STREQUAL "i386" )
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf")
  set(OBJCOPY_TARGET "elf32-i386")
  set(CAPABS "${CAPABS} -m32")
  enable_language(ASM_NASM)
elseif ("${ARCH}" STREQUAL "aarch64")
  #In cmake we trust
else()
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf64")
  set(OBJCOPY_TARGET "elf64-x86-64")
  set(CAPABS "${CAPABS} -m64")
  enable_language(ASM_NASM)
endif()

#TODO these should all come from the conan profile ?

# initialize C and C++ compiler flags
if (NOT ${PLATFORM} STREQUAL "userspace")
  if (CMAKE_COMPILER_IS_GNUCC)
    # gcc/g++ settings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CAPABS} ${WARNS} -Wno-frame-address -nostdlib -fno-omit-frame-pointer -c")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CAPABS} ${WARNS}  -nostdlib -fno-omit-frame-pointer -c")
  else()
    # these kinda work with llvm
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CAPABS} ${WARNS} -nostdlib -nostdlibinc -fno-omit-frame-pointer -c")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CAPABS} ${WARNS} -nostdlib  -nostdlibinc -fno-omit-frame-pointer -c")
  endif()
endif()

add_subdirectory(src)

# Install cmake files
install(FILES cmake/linux.service.cmake DESTINATION cmake)
install(FILES cmake/os.cmake DESTINATION cmake)

install(DIRECTORY api/ DESTINATION include/os)

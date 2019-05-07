#includeos standard settings for compilation and linkers

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

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

#TODO improve this to get the platform from conan somehow
if (NOT PLATFORM)
  set(PLATFORM "default")
endif()

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

cmake_minimum_required(VERSION 2.8.9)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CONAN_EXPORTED) # in conan local cache
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
  #TODO get this from an includeos header only package!!
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../api)
else()
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../api)
endif(CONAN_EXPORTED)

#only on x86_64
enable_language(ASM_NASM)

#do we need all of this*?
add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
add_definitions(-DPLATFORM_${PLATFORM})
add_definitions(-D__includeos__)

add_custom_command(
  PRE_BUILD
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/hotswap64.bin
	COMMAND ${CMAKE_ASM_NASM_COMPILER} -f bin -o ${CMAKE_CURRENT_BINARY_DIR}/hotswap64.bin ${CMAKE_CURRENT_SOURCE_DIR}/hotswap64.asm
  COMMENT "Building hotswap binary"
  DEPENDS hotswap64.asm
)
add_custom_target(hotswap64 DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hotswap64.bin)

enable_language(ASM_NASM)

#is this really neccesary ?
#add_custom_command(PRE_BUILD
#  PRE_BUILD
#  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/hotswap64_blob.o
#  COMMAND ${CMAKE_ASM_NASM_COMPILER} -f elf -o ${CMAKE_CURRENT_BINARY_DIR}/hotswap64_blob.o ${CMAKE_CURRENT_SOURCE_DIR}/hotswap64_blob.asm
#  COMMENT "Building hotswap object"
#  DEPENDS hotswap64
#)


set(SRCS
  storage.cpp
  partition.cpp
  update.cpp
  resume.cpp
  rollback.cpp
  elfscan.cpp
  os.cpp
  hotswap.cpp
  serialize_tcp.cpp
  hotswap64_blob.asm
)

if (${ARCH} STREQUAL "x86_64")
  list(APPEND SRCS "serialize_s2n.cpp")
endif()


# LiveUpdate static library
add_library(liveupdate STATIC ${SRCS} )
add_dependencies(liveupdate hotswap64)

if (NOT CONAN_EXPORTED)
  target_include_directories(liveupdate PUBLIC ${INCLUDEOS_ROOT}/api/posix)
  target_include_directories(liveupdate PUBLIC ${INCLUDEOS_ROOT}/src/include)
  target_include_directories(liveupdate PUBLIC ${INCLUDEOS_ROOT}/api)
  if (${ARCH} STREQUAL "x86_64")
    target_include_directories(liveupdate PUBLIC ${S2N_INCLUDE})
  endif()
  set(PREFIX ${ARCH}/)
endif()


install(TARGETS liveupdate DESTINATION ${PREFIX}lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/liveupdate.hpp DESTINATION ${PREFIX}include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/liveupdate DESTINATION ${PREFIX}include)

cmake_minimum_required(VERSION 2.8.9)

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
add_definitions(-DPLATFORM_${PLATFORM})
add_definitions(-D__includeos__)

include_directories(${INCLUDEOS_ROOT}/api/posix)
include_directories(${LIBCXX_INCLUDE_DIR})
include_directories(${MUSL_INCLUDE_DIR})
include_directories(${INCLUDEOS_ROOT}/src/include)
include_directories(${INCLUDEOS_ROOT}/api)
include_directories(${INCLUDEOS_ROOT}/mod/GSL/)
if (${ARCH} STREQUAL "x86_64")
  include_directories(${OPENSSL_DIR}/include)
endif()

add_custom_command(
  OUTPUT hotswap64.bin
	COMMAND ${CMAKE_ASM_NASM_COMPILER} -f bin -o hotswap64.bin ${CMAKE_CURRENT_SOURCE_DIR}/hotswap64.asm
	DEPENDS hotswap64.asm
)
add_custom_target(hotswap64 DEPENDS hotswap64.bin)

if(${ARCH} STREQUAL "x86_64")
  set(LIU_OPENSSL_FILES "serialize_openssl.cpp")
endif()

# LiveUpdate static library
add_library(liveupdate STATIC
    storage.cpp partition.cpp update.cpp resume.cpp rollback.cpp
    elfscan.cpp os.cpp "serialize_tcp.cpp"
    hotswap.cpp "hotswap64_blob.asm"
    ${LIU_OPENSSL_FILES}
  )
add_dependencies(liveupdate hotswap64)
add_dependencies(liveupdate PrecompiledLibraries)

install(TARGETS liveupdate DESTINATION includeos/${ARCH}/lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/liveupdate.hpp DESTINATION includeos/include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/liveupdate DESTINATION includeos/include)

cmake_minimum_required(VERSION 2.8.9)

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
add_definitions(-DPLATFORM_${PLATFORM})
add_definitions(-D__includeos__)

add_custom_command(
  OUTPUT hotswap64.bin
	COMMAND ${CMAKE_ASM_NASM_COMPILER} -f bin -o hotswap64.bin ${CMAKE_CURRENT_SOURCE_DIR}/hotswap64.asm
	DEPENDS hotswap64.asm
)
add_custom_target(hotswap64 DEPENDS hotswap64.bin)

# LiveUpdate static library
add_library(liveupdate STATIC
    storage.cpp partition.cpp update.cpp resume.cpp rollback.cpp
    elfscan.cpp os.cpp "serialize_tcp.cpp"
    hotswap.cpp "hotswap64_blob.asm"
  )
add_dependencies(liveupdate hotswap64)
add_dependencies(liveupdate PrecompiledLibraries)

target_include_directories(liveupdate PUBLIC ${INCLUDEOS_ROOT}/api/posix)
target_include_directories(liveupdate PUBLIC ${LIBCXX_INCLUDE_DIR})
target_include_directories(liveupdate PUBLIC ${MUSL_INCLUDE_DIR})
target_include_directories(liveupdate PUBLIC ${INCLUDEOS_ROOT}/src/include)
target_include_directories(liveupdate PUBLIC ${INCLUDEOS_ROOT}/api)
target_include_directories(liveupdate PUBLIC ${INCLUDEOS_ROOT}/mod/GSL/)
if (${ARCH} STREQUAL "x86_64")
  target_include_directories(liveupdate PUBLIC ${S2N_INCLUDE})
endif()

install(TARGETS liveupdate DESTINATION includeos/${ARCH}/lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/liveupdate.hpp DESTINATION includeos/include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/liveupdate DESTINATION includeos/include)

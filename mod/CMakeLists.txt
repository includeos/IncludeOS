set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf")
enable_language(ASM_NASM)

include_directories(${INCLUDEOS_ROOT}/api/posix)
include_directories(${LIBCXX_INCLUDE_DIR})
include_directories(${NEWLIB_INCLUDE_DIR})
include_directories(${INCLUDEOS_ROOT}/src/include)
include_directories(${INCLUDEOS_ROOT}/api)

# uzlib
FILE(GLOB UZLIB_SOURCES uzlib/src/*.c)
install(FILES uzlib/src/tinf.h DESTINATION includeos/include)

set(MOD_OBJECTS
  http-parser/http_parser.c
  ${UZLIB_SOURCES}
)

add_library(osdeps STATIC ${MOD_OBJECTS})

install(TARGETS osdeps DESTINATION includeos/lib)
add_dependencies(osdeps PrecompiledLibraries)

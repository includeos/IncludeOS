# Download and install precompiled bundle of botan (for includeos)
# https://github.com/randombit/botan

include(ExternalProject)

if(${ARCH} STREQUAL "x86_64")
  set(BOTAN_HASH 707bb3ce80c8fbd8bf84e15211f68936)
elseif(${ARCH} STREQUAL "i686")
  set(BOTAN_HASH 788fbe7e214d22638135c5a603bc0fe7)
endif()

ExternalProject_Add(botan
        PREFIX botan
        URL https://github.com/includeos/botan/releases/download/inc-3.0/botan-includeos-${ARCH}.tar.gz
        URL_HASH MD5=${BOTAN_HASH}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
)


set(BOTAN_DIR ${CMAKE_CURRENT_BINARY_DIR}/botan/src/botan)
set(BOTAN_INCLUDE ${BOTAN_DIR}/botan)
set(BOTAN_LIB ${BOTAN_DIR}/libbotan-2.a)

add_library(libbotan STATIC IMPORTED)
#add_dependencies(libcxx PrecompiledLibraries)
set_target_properties(libbotan PROPERTIES IMPORTED_LOCATION ${BOTAN_LIB})

install(FILES ${BOTAN_LIB} DESTINATION includeos/${ARCH}/lib)
install(DIRECTORY ${BOTAN_INCLUDE} DESTINATION includeos/${ARCH}/include)

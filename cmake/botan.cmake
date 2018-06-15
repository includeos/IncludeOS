# Download and install precompiled bundle of botan (for includeos)
# https://github.com/randombit/botan

include(ExternalProject)

if(${ARCH} STREQUAL "x86_64")
  set(BOTAN_HASH 6f0a3e4aaf6723d83fd025176249372b)
elseif(${ARCH} STREQUAL "i686")
  set(BOTAN_HASH 0175670a61b45a04f832ee9e5965f1f3)
endif()

ExternalProject_Add(botan
        PREFIX botan
        URL https://github.com/includeos/botan/releases/download/musl-1.1/botan-includeos-${ARCH}.tar.gz
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

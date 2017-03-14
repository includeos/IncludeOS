# Download and install precompiled bundle of botan (for includeos)
# https://github.com/randombit/botan

include(ExternalProject)
ExternalProject_Add(botan
        PREFIX botan
        URL https://github.com/includeos/botan/releases/download/inc-2.0/botan-includeos.tar.gz
        URL_HASH MD5=5ef7f26047f8fe17219f62755938621d
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

install(FILES ${BOTAN_LIB} DESTINATION includeos/lib)
install(DIRECTORY ${BOTAN_INCLUDE} DESTINATION includeos/include)

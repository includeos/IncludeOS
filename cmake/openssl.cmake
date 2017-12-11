# Download and install bundle of OpenSSL
include(ExternalProject)

if(${ARCH} STREQUAL "x86_64")
  set(OPENSSL_HASH 8f807887c7f9835a7fc4a54b0d3cce55)

  ExternalProject_Add(openssl_bundle
          PREFIX openssl
          URL https://github.com/fwsGonzo/OpenSSL_bundle/releases/download/v1.0/openssl_bundle.tar.gz
          URL_HASH MD5=${OPENSSL_HASH}
          CONFIGURE_COMMAND ""
          BUILD_COMMAND ""
          UPDATE_COMMAND ""
          INSTALL_COMMAND ""
  )

  set(OPENSSL_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl/src/openssl_bundle)
  set(OPENSSL_INCLUDE ${OPENSSL_DIR}/include)
  set(OPENSSL_CONF    ${OPENSSL_DIR}/openssl/opensslconf.h)
  set(OPENSSL_LIB_CRYPTO ${OPENSSL_DIR}/libcrypto.a)
  set(OPENSSL_LIB_SSL    ${OPENSSL_DIR}/libssl.a)

  add_library(openssl_ssl STATIC IMPORTED)
  set_target_properties(openssl_ssl PROPERTIES IMPORTED_LOCATION ${OPENSSL_LIB_SSL})
  add_library(openssl_crypto STATIC IMPORTED)
  set_target_properties(openssl_crypto PROPERTIES IMPORTED_LOCATION ${OPENSSL_LIB_CRYPTO})

  install(FILES ${OPENSSL_LIB_CRYPTO}  DESTINATION includeos/${ARCH}/lib)
  install(FILES ${OPENSSL_LIB_SSL}     DESTINATION includeos/${ARCH}/lib)
  install(DIRECTORY ${OPENSSL_INCLUDE} DESTINATION includeos/${ARCH}/include)
  install(FILES ${OPENSSL_CONF}        DESTINATION includeos/${ARCH}/include/openssl)
endif()

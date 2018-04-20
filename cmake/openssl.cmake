# Download and install bundle of OpenSSL
include(ExternalProject)

if(${ARCH} STREQUAL "x86_64")
  set(OPENSSL_HASH 3ef6bc4e8be049725ca3887f0d235031)

  ExternalProject_Add(openssl_bundle
          PREFIX openssl
          URL https://github.com/fwsGonzo/OpenSSL_bundle/releases/download/v1.2/openssl_bundle.tar.gz
          URL_HASH MD5=${OPENSSL_HASH}
          CONFIGURE_COMMAND ""
          BUILD_COMMAND ""
          UPDATE_COMMAND ""
          INSTALL_COMMAND ""
  )

  set(OPENSSL_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl/src/openssl_bundle)
  set(OPENSSL_INCLUDE ${OPENSSL_DIR}/include)
  set(OPENSSL_LIB_CRYPTO ${OPENSSL_DIR}/lib/libcrypto.a)
  set(OPENSSL_LIB_SSL    ${OPENSSL_DIR}/lib/libssl.a)

  add_library(openssl_ssl STATIC IMPORTED)
  set_target_properties(openssl_ssl PROPERTIES IMPORTED_LOCATION ${OPENSSL_LIB_SSL})
  add_library(openssl_crypto STATIC IMPORTED)
  set_target_properties(openssl_crypto PROPERTIES IMPORTED_LOCATION ${OPENSSL_LIB_CRYPTO})

  install(FILES ${OPENSSL_LIB_CRYPTO}  DESTINATION includeos/${ARCH}/lib)
  install(FILES ${OPENSSL_LIB_SSL}     DESTINATION includeos/${ARCH}/lib)
  install(DIRECTORY ${OPENSSL_INCLUDE} DESTINATION includeos/${ARCH})
endif()

ExternalProject_Add(cert_bundle
        PREFIX cert_bundle
        URL https://github.com/fwsGonzo/OpenSSL_bundle/releases/download/v1.2/ca_bundle.tar.gz
        URL_HASH MD5=4596f90b912bea7ad7bd974d10c58efd
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
)
set(CERT_BUNDLE_DIR ${CMAKE_CURRENT_BINARY_DIR}/cert_bundle/src/cert_bundle)
install(DIRECTORY ${CERT_BUNDLE_DIR} DESTINATION includeos/)

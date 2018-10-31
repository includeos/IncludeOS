# Download and install bundle of OpenSSL
include(ExternalProject)

if(${ARCH} STREQUAL "x86_64")
  ExternalProject_Add(s2n_bundle
          PREFIX s2n
          URL https://github.com/fwsGonzo/s2n_bundle/releases/download/v1/s2n_bundle.tar.gz
          URL_HASH MD5=657af3e79e3a8ef08e2be5150386d193
          CONFIGURE_COMMAND ""
          BUILD_COMMAND ""
          UPDATE_COMMAND ""
          INSTALL_COMMAND ""
  )

  set(S2N_DIR ${CMAKE_CURRENT_BINARY_DIR}/s2n/src/s2n_bundle)
  set(S2N_INCLUDE    ${S2N_DIR}/include)
  set(S2N_LIB_CRYPTO ${S2N_DIR}/lib/libcrypto.a)
  set(S2N_LIB_SSL    ${S2N_DIR}/lib/libssl.a)
  set(S2N_LIB_S2N    ${S2N_DIR}/lib/libs2n.a)

  add_library(s2n_crypto STATIC IMPORTED)
  set_target_properties(s2n_crypto PROPERTIES IMPORTED_LOCATION ${S2N_LIB_CRYPTO})
  add_library(s2n_libssl STATIC IMPORTED)
  set_target_properties(s2n_libssl PROPERTIES IMPORTED_LOCATION ${S2N_LIB_SSL})
  add_library(s2n_libs2n STATIC IMPORTED)
  set_target_properties(s2n_libs2n PROPERTIES IMPORTED_LOCATION ${S2N_LIB_S2N})

  install(FILES ${S2N_LIB_CRYPTO}  DESTINATION includeos/${ARCH}/lib)
  install(FILES ${S2N_LIB_SSL}     DESTINATION includeos/${ARCH}/lib)
  install(FILES ${S2N_LIB_S2N}     DESTINATION includeos/${ARCH}/lib)
  install(DIRECTORY ${S2N_INCLUDE} DESTINATION includeos/${ARCH})
endif()

ExternalProject_Add(cert_bundle
        PREFIX cert_bundle
        URL https://github.com/fwsGonzo/s2n_bundle/releases/download/v1/ca_bundle.tar.gz
        URL_HASH MD5=4596f90b912bea7ad7bd974d10c58efd
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
)
set(CERT_BUNDLE_DIR ${CMAKE_CURRENT_BINARY_DIR}/cert_bundle/src/cert_bundle)
install(DIRECTORY ${CERT_BUNDLE_DIR} DESTINATION includeos/)

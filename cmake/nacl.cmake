include(ExternalProject)

set(NACL_HASH 653b85599705c7932ffd762bd4884fb1)
ExternalProject_Add(nacl_bin
  PREFIX nacl_bin
  URL https://github.com/includeos/NaCl/releases/download/v0.1.0/nacl_bin.tar.gz
  URL_HASH MD5=${NACL_HASH}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
)

## Placed in install_dependencies_*.sh
#execute_process(COMMAND "pip" "install" "--user" "pystache")
#execute_process(COMMAND "pip" "install" "--user" "antlr4-python2-runtime")

set(NACL_DIR ${INCLUDEOS_ROOT}/NaCl)
set(NACL_EXE ${NACL_DIR}/NaCl.py)
set(NACL_SRC
  ${NACL_DIR}/cpp_template.mustache
  ${NACL_DIR}/shared.py
  )
set(NACL_SUBTRANS_SRC
  ${NACL_DIR}/subtranspilers/__init__.py
  ${NACL_DIR}/subtranspilers/function_transpiler.py
  ${NACL_DIR}/subtranspilers/value_transpiler.py
  )
set(NACL_TYPEPROC_SRC
  ${NACL_DIR}/type_processors/__init__.py
  ${NACL_DIR}/type_processors/conntrack.py
  ${NACL_DIR}/type_processors/function.py
  ${NACL_DIR}/type_processors/gateway.py
  ${NACL_DIR}/type_processors/iface.py
  ${NACL_DIR}/type_processors/load_balancer.py
  ${NACL_DIR}/type_processors/syslog.py
  )
set(NACL_BIN ${CMAKE_CURRENT_BINARY_DIR}/nacl_bin/src/nacl_bin)

install(PROGRAMS ${NACL_EXE} DESTINATION includeos/nacl)
install(FILES ${NACL_SRC} DESTINATION includeos/nacl)
install(FILES ${NACL_SUBTRANS_SRC} DESTINATION includeos/nacl/subtranspilers)
install(FILES ${NACL_TYPEPROC_SRC} DESTINATION includeos/nacl/type_processors)
install(DIRECTORY ${NACL_BIN}/ DESTINATION includeos/nacl)

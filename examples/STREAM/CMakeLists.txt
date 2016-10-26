set(SERVICE_NAME "\"\\\"Dual NIC Demo Service\\\"\"")
set(SERVICE "STREAM_Service")
set(SERVICE_ID "STREAM")

set(SERVICE_SOURCES service.cpp stream.cpp ${INCLUDEOS_INSTALL_DIR}/service_name.cpp)

add_example(${SERVICE_ID} ${SERVICE} ${SERVICE_NAME} "${SERVICE_SOURCES}")

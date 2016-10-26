set(SERVICE_NAME "\"\\\"Dual NIC Demo Service\\\"\"")
set(SERVICE "snake_service")
set(SERVICE_ID "snake")

set(SERVICE_SOURCES service.cpp ${INCLUDEOS_INSTALL_DIR}/service_name.cpp)

add_example(${SERVICE_ID} ${SERVICE} ${SERVICE_NAME} "${SERVICE_SOURCES}")

set(SERVICE_NAME "\"\\\"Dual NIC Demo Service\\\"\"")
set(SERVICE "tcp_Service")
set(SERVICE_ID "tcp")

set(SERVICE_SOURCES service.cpp ${SERVICE_STUB})

add_example(${SERVICE_ID} ${SERVICE} ${SERVICE_NAME} "${SERVICE_SOURCES}")

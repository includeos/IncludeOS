set(SERVICE_NAME "\"\\\"Dual NIC Demo Service\\\"\"")
set(SERVICE "dualnic_service")
set(SERVICE_ID "dualnic")

set(SERVICE_SOURCES service.cpp ${SERVICE_STUB})

add_example(${SERVICE_ID} ${SERVICE} ${SERVICE_NAME} "${SERVICE_SOURCES}")

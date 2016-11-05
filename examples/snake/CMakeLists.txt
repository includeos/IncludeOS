set(SERVICE_NAME "\"\\\"Dual NIC Demo Service\\\"\"")
set(SERVICE "snake_service")
set(SERVICE_ID "snake")

set(SERVICE_SOURCES service.cpp ${SERVICE_STUB})

add_example(${SERVICE_ID} ${SERVICE} ${SERVICE_NAME} "${SERVICE_SOURCES}")

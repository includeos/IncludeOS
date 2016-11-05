set(SERVICE_NAME "\"\\\"IncludeOS Demo Service\\\"\"")
set(SERVICE "IncludeOS_Demo_Service")
set(SERVICE_ID "demo_service")

set(SERVICE_SOURCES service.cpp ${SERVICE_STUB})

add_example(${SERVICE_ID} ${SERVICE} ${SERVICE_NAME} "${SERVICE_SOURCES}")

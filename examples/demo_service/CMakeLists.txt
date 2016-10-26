set(SERVICE_NAME "\"\\\"IncludeOS Demo Service\\\"\"")
set(SERVICE "IncludeOS_Demo_Service")
set(SERVICE_ID "demo_service")

set(SERVICE_SOURCES service.cpp ${INCLUDEOS_INSTALL_DIR}/service_name.cpp)

add_example(${SERVICE_ID} ${SERVICE} ${SERVICE_NAME} "${SERVICE_SOURCES}")

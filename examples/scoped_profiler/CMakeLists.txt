set(SERVICE_NAME "\"\\\"IncludeOS Scoped Profiler Demo\\\"\"")
set(SERVICE "IncludeOS_Profiler_Demo")
set(SERVICE_ID "scoped_profiler")

set(SERVICE_SOURCES service.cpp ${INCLUDEOS_INSTALL_DIR}/service_name.cpp)

add_example(${SERVICE_ID} ${SERVICE} ${SERVICE_NAME} "${SERVICE_SOURCES}")

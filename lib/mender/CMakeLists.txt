cmake_minimum_required(VERSION 2.8.9)

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

include_directories(${INCLUDEOS_ROOT}/api/posix)
include_directories(${INCLUDEOS_ROOT}/src/include)
include_directories(${INCLUDEOS_ROOT}/api)

#dependencies
include_directories(${INCLUDEOS_ROOT}/lib/LiveUpdate)

#this is me
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(LIBRARY_NAME "mender")

set(SOURCES
  src/artifact.cpp
  src/auth_manager.cpp
  src/client.cpp
  src/keystore.cpp
  src/state.cpp
  )

add_library(${LIBRARY_NAME} STATIC ${SOURCES})

# verbose mender or not
target_compile_definitions(${LIBRARY_NAME} PRIVATE VERBOSE_MENDER=1)

install(TARGETS ${LIBRARY_NAME} DESTINATION includeos/${ARCH}/lib)
install(DIRECTORY include/mender DESTINATION includeos/include)

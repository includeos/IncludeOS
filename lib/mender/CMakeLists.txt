cmake_minimum_required(VERSION 2.8.9)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CONAN_EXPORTED) # in conan local cache
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()

  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../api)

  #TODO GET ARCH FROM CONAN ?

else()
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
  
  include_directories(${INCLUDEOS_ROOT}/api/posix)
  include_directories(${INCLUDEOS_ROOT}/src/include)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../api)

  #dependencies
  include_directories(${INCLUDEOS_ROOT}/lib/LiveUpdate)
  set(INCLUDE_PREFIX "includeos/")
  set(LIB_PREFIX "includeos/${ARCH}/")
endif()


add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
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

install(TARGETS ${LIBRARY_NAME} DESTINATION ${LIB_PREFIX}lib)
install(DIRECTORY include/mender DESTINATION ${INCLUDE_PREFIX}include)

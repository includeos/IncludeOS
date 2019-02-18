cmake_minimum_required(VERSION 2.8.9)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

#include self
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CONAN_EXPORTED) # in conan local cache
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
  #TODO get this from includeos package.. and not here!!
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../api)
else(CONAN_EXPORTED)
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
  #TODO get from conan the right include path
  #include_directories()
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../api)
  include_directories(${INCLUDEOS_ROOT}/api/posix)
  include_directories(${INCLUDEOS_ROOT}/src/include)
  include_directories(${INCLUDEOS_ROOT}/api)
  set(INCLUDE_PREFIX "includeos/")
  set(LIB_PREFIX "includeos/${ARCH}/")
endif()



set(MANA_OBJ
  src/request.cpp
  src/response.cpp
  src/server.cpp
  )

set(MANA_COMP
  src/components/dashboard/dashboard.cpp
  )

set(MANA_MWARE
  src/middleware/butler.cpp
  src/middleware/director.cpp
  src/middleware/parsley.cpp
  src/middleware/cookie_parser.cpp
  )

set(MANA_ATTR
  src/attributes/cookie_jar.cpp
  )

add_library(mana STATIC ${MANA_OBJ} ${MANA_COMP} ${MANA_MWARE} ${MANA_ATTR})

install(TARGETS mana DESTINATION ${LIB_PREFIX}lib)
install(DIRECTORY include/mana DESTINATION ${INCLUDE_PREFIX}include)

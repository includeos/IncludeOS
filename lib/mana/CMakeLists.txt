cmake_minimum_required(VERSION 2.8.9)

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

set(LIB_MANA ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${LIB_MANA}/include)


if(CONAN_EXPORTED) # in conan local cache
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
endif(CONAN_EXPORTED)
#TODO get from conan the right include path
include_directories()

include_directories(${INCLUDEOS_ROOT}/api/posix)
include_directories(${INCLUDEOS_ROOT}/src/include)
include_directories(${INCLUDEOS_ROOT}/api)



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

#add_dependencies(mana PrecompiledLibraries)

install(TARGETS mana DESTINATION includeos/${ARCH}/lib)
install(DIRECTORY include/mana DESTINATION includeos/include)

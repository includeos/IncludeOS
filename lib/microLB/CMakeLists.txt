cmake_minimum_required(VERSION 2.8.9)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CONAN_EXPORTED) # in conan local cache
  # standard conan installation, deps will be defined in conanfile.py
  # and not necessary to call conan again, conan is already running
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
endif()

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

set(LIBRARY_SRCS
  micro_lb/autoconf.cpp
  micro_lb/balancer.cpp

)

if (TSL)
  list(APPEND LIBRARY_SRCS
    micro_lb/openssl.cpp
  )
endif()


if (LIVEUPDATE)
  set_target_properties(${LIBRARY_SRCS} PROPERTIES COMPILE_DEFINITIONS "LIVEUPDATE")
  list(APPEND LIBRARY_SRCS
    micro_lb/serialize.cpp
  )
endif()

# microLB static library
add_library(microlb STATIC ${LIBRARY_SRCS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../api)
target_include_directories(microlb PUBLIC ${INCLUDEOS_ROOT}/api/posix)
target_include_directories(microlb PUBLIC ${INCLUDEOS_ROOT}/src/include)
target_include_directories(microlb PUBLIC ${INCLUDEOS_ROOT}/api)
target_include_directories(microlb PUBLIC ${INCLUDEOS_ROOT}/lib/LiveUpdate)

if(NOT CONAN_EXPORTED)
  SET(PREFIX ${ARCH}/)
endif()

install(TARGETS microlb DESTINATION ${PREFIX}lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/micro_lb/balancer.hpp DESTINATION ${PREFIX}include/micro_lb)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/microLB DESTINATION ${PREFIX}include)

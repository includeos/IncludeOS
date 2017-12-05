####################################
#   Linux Userspace CMake script   #
####################################

set(CMAKE_CXX_STANDARD 14)
set(COMMON "-g -O2 -march=native -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON}")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON}")

option(GPROF "Enable profiling with gprof" OFF)
option(SANITIZE "Enable undefined- and address sanitizers" OFF)
option(ENABLE_LTO "Enable thinLTO for use with LLD" OFF)
option(CUSTOM_BOTAN "Enable building with a local Botan" OFF)

if (ENABLE_LTO)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin -fuse-ld=lld-5.0")
  set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin -fuse-ld=lld-5.0")
endif()

if(GPROF)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

if(SANITIZE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fsanitize=address")
endif()

if(CUSTOM_BOTAN)
  include_directories("/usr/local/botan/include/botan-2")
endif()

add_definitions(-DARCH="x86_64" -DARCH_x86_64)
add_definitions(-DOS_TERMINATE_ON_CONTRACT_VIOLATION)
add_definitions(-DARP_PASSTHROUGH)
add_definitions(-DNO_DEBUG)
add_definitions(-DINCLUDEOS_SINGLE_THREADED)
add_definitions(-DSERVICE=\"\\\"${BINARY}\\\"\")
add_definitions(-DSERVICE_NAME=\"\\\"${SERVICE_NAME}\\\"\")
add_definitions(-DUSERSPACE_LINUX)

set(IOSPATH $ENV{INCLUDEOS_PREFIX}/includeos)

# includes
include_directories(${LOCAL_INCLUDES})
include_directories(${IOSPATH}/x86_64/include)
include_directories(${IOSPATH}/api)
include_directories(${IOSPATH}/include)
include_directories(${IOSPATH}/linux)
include_directories(${IOSPATH}/../include)

set(LPATH $ENV{INCLUDEOS_PREFIX}/includeos/linux)

add_library(linuxrt STATIC IMPORTED)
set_target_properties(linuxrt PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(linuxrt PROPERTIES IMPORTED_LOCATION ${LPATH}/liblinuxrt.a)

add_library(includeos STATIC IMPORTED)
set_target_properties(includeos PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(includeos PROPERTIES IMPORTED_LOCATION ${LPATH}/libincludeos.a)

add_library(http_parser STATIC IMPORTED)
set_target_properties(http_parser PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(http_parser PROPERTIES IMPORTED_LOCATION ${LPATH}/libhttp_parser.a)

add_executable(service ${SOURCES} ${IOSPATH}/src/service_name.cpp)
set_target_properties(service PROPERTIES OUTPUT_NAME ${BINARY})

if (CUSTOM_BOTAN)
  target_link_libraries(service /usr/local/botan/lib/libbotan-2.a -ldl -pthread)
endif()
target_link_libraries(service ${EXTRA_LIBS})
target_link_libraries(service includeos linuxrt includeos http_parser rt)

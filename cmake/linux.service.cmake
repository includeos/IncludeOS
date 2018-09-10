####################################
#   Linux Userspace CMake script   #
####################################

#set(CMAKE_CXX_STANDARD 17)
set(COMMON "-g -O2 -march=native -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ${COMMON}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON}")

option(DEBUGGING "Enable debugging" OFF)
option(GPROF "Enable profiling with gprof" OFF)
option(SANITIZE "Enable undefined- and address sanitizers" OFF)
option(ENABLE_LTO "Enable thinLTO for use with LLD" OFF)
option(CUSTOM_BOTAN "Enable building with a local Botan" OFF)
option(STATIC_BUILD "Build a portable static executable" ON)
option(STRIP_BINARY "Strip final binary to reduce size" OFF)
option(USE_LLD "Allow linking against LTO archives" ON)

if(DEBUGGING)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
endif()

if (ENABLE_LTO)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin")
  set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin")
endif()

if(GPROF)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

if(SANITIZE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fsanitize=address")
endif()

if(CUSTOM_BOTAN)
  include_directories("/usr/local/include/botan/botan-2")
endif()

set(ARCH "x86_64")
add_definitions("-DARCH=${ARCH}" "-DARCH_${ARCH}")
add_definitions(-DOS_TERMINATE_ON_CONTRACT_VIOLATION)
add_definitions(-DARP_PASSTHROUGH)
add_definitions(-DNO_DEBUG)
add_definitions(-DSERVICE=\"\\\"${BINARY}\\\"\")
add_definitions(-DSERVICE_NAME=\"\\\"${SERVICE_NAME}\\\"\")
add_definitions(-DUSERSPACE_LINUX)

set(IOSPATH $ENV{INCLUDEOS_PREFIX}/includeos)

# includes
include_directories(${LOCAL_INCLUDES})
include_directories(${IOSPATH}/${ARCH}/include)
include_directories(${IOSPATH}/api)
include_directories(${IOSPATH}/include)
include_directories(${IOSPATH}/linux)
include_directories(${IOSPATH}/../include)

# linux executable
add_executable(service ${SOURCES} ${IOSPATH}/src/service_name.cpp)
set_target_properties(service PROPERTIES OUTPUT_NAME ${BINARY})

set(LPATH ${IOSPATH}/linux)
set(PLUGIN_LOC "${IOSPATH}/linux/plugins")
set(DRIVER_LOC "${IOSPATH}/${ARCH}/drivers")

# IncludeOS plugins
set(PLUGINS_LIST)
function(configure_plugin type name path)
  add_library(${type}_${name} STATIC IMPORTED)
  set_target_properties(${type}_${name} PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(${type}_${name} PROPERTIES IMPORTED_LOCATION ${path})
  set(PLUGINS_LIST ${PLUGINS_LIST} -Wl,--whole-archive ${type}_${name} -Wl,--no-whole-archive PARENT_SCOPE)
endfunction()
foreach(PNAME ${PLUGINS})
  set(PPATH "${PLUGIN_LOC}/lib${PNAME}.a")
  message(STATUS "Enabling plugin: ${PNAME} --> ${PPATH}")
  configure_plugin("plugin" ${PNAME} ${PPATH})
endforeach()
foreach(DNAME ${DRIVERS})
  set(DPATH "${DRIVER_LOC}/lib${DNAME}.a")
  message(STATUS "Enabling driver: ${DNAME} --> ${DPATH}")
  configure_plugin("driver" ${DNAME} ${DPATH})
endforeach()

# static imported libraries
add_library(linuxrt STATIC IMPORTED)
set_target_properties(linuxrt PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(linuxrt PROPERTIES IMPORTED_LOCATION ${LPATH}/liblinuxrt.a)

add_library(includeos STATIC IMPORTED)
set_target_properties(includeos PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(includeos PROPERTIES IMPORTED_LOCATION ${LPATH}/libincludeos.a)

add_library(http_parser STATIC IMPORTED)
set_target_properties(http_parser PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(http_parser PROPERTIES IMPORTED_LOCATION ${LPATH}/libhttp_parser.a)

if (CUSTOM_BOTAN)
  set(BOTAN_LIBS /usr/local/lib/libbotan-2.a)
  target_link_libraries(service ${BOTAN_LIBS} -ldl -pthread)
endif()
target_link_libraries(service ${PLUGINS_LIST})
target_link_libraries(service includeos linuxrt includeos http_parser rt)
target_link_libraries(service ${EXTRA_LIBS})
if (CUSTOM_BOTAN)
  target_link_libraries(service ${BOTAN_LIBS})
endif()

if (STATIC_BUILD)
  target_link_libraries(service -static-libstdc++ -static-libgcc)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -pthread")
endif()

if (ENABLE_LTO OR USE_LLD)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
endif()

if (STRIP_BINARY)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
endif()

# write binary name to file
file(WRITE ${CMAKE_BINARY_DIR}/binary.txt ${BINARY})

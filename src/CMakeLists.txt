#
# CMake script for the OS library
#

#TODO restructure this in a different commit based on KJ/CMakeFixes branch
add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

if (SMP)
  add_definitions(-DINCLUDEOS_SMP_ENABLE)
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../api
  include
)

#TODO move to util and check if needed / can be changed ?..
include_directories(${INCLUDEOS_ROOT}/lib/LiveUpdate/include)

set(SRCS
  version.cpp
)
set(LIBRARIES
    hal
    kernel
    util
    net
    fs
    posix
    virtio
    hw
)

if (NOT CMAKE_TESTING_ENABLED)
  list(APPEND LIBRARIES crt)
endif()

if(${PLATFORM} STREQUAL "solo5-hvt")
  message(STATUS "Building for solo5")
  add_definitions(-DPLATFORM_x86_solo5)
endif()

SET(OBJECTS)
#TODO make the subdirectory add return an LIST of target objects..
#needed to make multilevel dep work for eg.. net.
foreach(LIB ${LIBRARIES})
    add_subdirectory(${LIB})
    list(APPEND OBJECTS "$<TARGET_OBJECTS:${LIB}>" )
endforeach()

if (CMAKE_TESTING_ENABLED)
  list(APPEND SRCS
    arch/${ARCH}/paging.cpp
  )
endif()

if (NOT VERSION)
  set(VERSION VERY_DIRTY)
endif()

FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.h
  "#define OS_VERSION \"${VERSION}\"\n"
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(os STATIC ${SRCS} ${OBJECTS} ${CMAKE_CURRENT_BINARY_DIR}/version.h)

#TODO check if this is almost correct for platform userspace
if (NOT CMAKE_TESTING_ENABLED)
  add_subdirectory(arch)
  add_subdirectory(platform)
  add_subdirectory(drivers)
  add_subdirectory(plugins)
  add_subdirectory(musl)
endif()

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam
install(TARGETS os DESTINATION lib)
if (NOT CMAKE_TESTING_ENABLED)
  configure_file(memdisk/empty.asm ${CMAKE_BINARY_DIR}/tools/memdisk/empty.asm)
  configure_file(memdisk/memdisk.asm ${CMAKE_BINARY_DIR}/tools/memdisk/memdisk.asm)
  configure_file(memdisk/memdisk.py ${CMAKE_BINARY_DIR}/tools/memdisk/memdisk.py)
endif()
#TODO build ?
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/memdisk/ DESTINATION tools/memdisk
        FILES_MATCHING PATTERN "*.*")

install(FILES service_name.cpp DESTINATION src)

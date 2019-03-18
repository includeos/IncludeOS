#
# CMake script for the OS library
#

#TODO restructure this in a different commit based on KJ/CMakeFixes branch
add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

if (smp)
  add_definitions(-DINCLUDEOS_SMP_ENABLE)
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../api
  include
)

#TODO move to util and check if needed / can be changed.. unwanted dependency?
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


SET(OBJECTS)
foreach(LIB ${LIBRARIES})
    add_subdirectory(${LIB})
    list(APPEND OBJECTS "$<TARGET_OBJECTS:${LIB}>" )
endforeach()

if (CMAKE_TESTING_ENABLED)
  list(APPEND SRCS
    arch/${ARCH}/paging.cpp
  )
endif()
add_library(os STATIC ${SRCS} ${OBJECTS})

if (NOT CMAKE_TESTING_ENABLED)
  add_dependencies(os version )
  add_subdirectory(arch/${ARCH})
  if (NOT CORE_OS)
    if(WITH_SOLO5)
      message(STATUS "platform x86_solo5")
      add_subdirectory(platform/x86_solo5)
    else()
      message(STATUS "platform x86_pc")
      add_subdirectory(platform/x86_pc)
    endif(WITH_SOLO5)

    add_subdirectory(drivers)
    add_subdirectory(plugins)
  else()
    if ("${ARCH}" STREQUAL "aarch64")
      message(STATUS "platform aarch64_vm")
      add_subdirectory(platform/aarch64_vm)
    else()
      message(STATUS "platform x86_nano")
      add_subdirectory(platform/x86_nano)
    endif()
  endif()
  # Add musl
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

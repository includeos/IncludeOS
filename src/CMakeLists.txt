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
include_directories(${INCLUDEOS_ROOT}/lib/LiveUpdate)



set(SRCS
  version.cpp
)
set(LIBRARIES
    fuzz
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
    SET(OBJECTS ${OBJECTS} "$<TARGET_OBJECTS:${LIB}>" )
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

    add_subdirectory(platform/x86_pc)

    if(WITH_SOLO5)
     add_subdirectory(platform/x86_solo5)
    endif(WITH_SOLO5)

    add_subdirectory(drivers)
    add_subdirectory(plugins)
  endif()
  add_subdirectory(platform/x86_nano)
  # Add musl
  add_subdirectory(musl)
endif()

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam
install(TARGETS os DESTINATION ${ARCH}/lib)

#TODO build ?
install(DIRECTORY ${INCLUDEOS_ROOT}/src/memdisk/ DESTINATION tools/memdisk
        FILES_MATCHING PATTERN "*.*")

install(FILES service_name.cpp DESTINATION src)

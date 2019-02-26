#set(CREATE_TAR ${CMAKE_SOURCE_DIR}/tar_example)
#set(TARFILE tar_example.tar)

cmake_minimum_required(VERSION 3.0)
# IncludeOS install location
if (NOT DEFINED INCLUDEOS_PREFIX)
  if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
    set(INCLUDEOS_PREFIX /usr/local/includeos)
  else()
    set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
  endif()
endif()

if (NOT EXISTS "${INCLUDEOS_PREFIX}/cmake/os.cmake")
  MESSAGE(FATAL_ERROR "IncludeOS does not appear to be installed at ${INCLUDEOS_PREFIX}")
endif()
list(APPEND CMAKE_MODULE_PATH ${INCLUDEOS_PREFIX}/cmake)

#service
project (tar_service)
include(os)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tar_example.tar
  COMMENT "Creating tar file"
  COMMAND tar cfv ${CMAKE_CURRENT_BINARY_DIR}/tar_example.tar -C ${CMAKE_CURRENT_SOURCE_DIR} tar_example
  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/tar_example.tar
)


os_add_executable(util_tar "Tar Test Service" service.cpp)
os_add_binary_blob(util_tar ${CMAKE_CURRENT_BINARY_DIR}/tar_example.tar input.bin binary)
os_add_stdout(util_tar default_stdout)
os_add_drivers(util_tar virtionet)

#add_custom_target(tarfile ALL COMMAND
#    ${CMAKE_COMMAND} -E tar "cfvz" "executables.tgz" "${EXECUTABLE_OUTPUT_PATH}")
#add_dependencies(create_tar foo)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

#COMMAND tar cf ${TAR_RELPATH} -C ${CMAKE_SOURCE_DIR} ${TAR_BASE_NAME}
#COMMAND cp ${TAR_RELPATH} input.bin
#COMMAND ${CMAKE_OBJCOPY} -I binary -O ${OBJCOPY_TARGET} -B i386 input.bin tarfile.o
#COMMAND rm input.bin
#)
#elseif(CREATE_TAR_GZ)
#et_filename_component(TAR_BASE_NAME "${CREATE_TAR_GZ}" NAME)
#add_custom_command(
#OUTPUT tarfile.o
#COMMAND tar czf ${TAR_RELPATH} -C ${CMAKE_SOURCE_DIR} ${TAR_BASE_NAME}

cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (tar_service)


include(os)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tar_example.tgz
  COMMENT "Creating tar file"
  COMMAND tar cfvz ${CMAKE_CURRENT_BINARY_DIR}/tar_example.tgz -C ${CMAKE_CURRENT_SOURCE_DIR} tar_example
  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/tar_example.tgz
)

os_add_executable(util_tar_gz "Tar.gz Test Service" service.cpp)

os_add_binary_blob(util_tar_gz ${CMAKE_CURRENT_BINARY_DIR}/tar_example.tgz input.bin binary)
os_add_stdout(util_tar_gz default_stdout)
os_add_drivers(util_tar_gz virtionet)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})

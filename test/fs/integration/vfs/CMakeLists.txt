cmake_minimum_required(VERSION 3.0)

project(vfs)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

include(os)

os_add_executable(fs_vfs "VFS filesystem test" service.cpp)
os_add_stdout(fs_vfs default_stdout)
os_add_drivers(fs_vfs virtioblk)
os_diskbuilder(fs_vfs memdisk)

#do this code in cmake instead ?
file(COPY memdisk DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY virtio1 DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY virtio2 DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY create_disk.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(vm.json ${CMAKE_CURRENT_BINARY_DIR})

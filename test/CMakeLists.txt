cmake_minimum_required(VERSION 2.8.9)

project(unittests CXX)

set(CMAKE_CXX_FLAGS "-m32 -std=c++14 -Wall -Wextra -Wno-unused-function -DNO_INFO=1 -DGSL_THROW_ON_CONTRACT_VIOLATION -Dlest_FEATURE_AUTO_REGISTER=1 -DHAVE_LEST_MAIN")
set(SRC ${INCLUDEOS_ROOT}/src)
set(TEST ${INCLUDEOS_ROOT}/test)

include_directories(
  ${INCLUDEOS_ROOT}/api
  ${INCLUDEOS_ROOT}/src/
  ${INCLUDEOS_ROOT}/src/include
  ${INCLUDEOS_ROOT}/mod/GSL
  ${TEST}/lest/include
  ${TEST}/lest_util
)

set(TEST_SOURCES
  ${TEST}/lest_util/lestmain.cxx
  ${TEST}/lest_util/os_mock.cpp
  ${TEST}/fs/unit/path_test.cpp
  ${TEST}/fs/unit/vfs_test.cpp
  ${TEST}/hw/unit/mac_addr_test.cpp
  ${TEST}/kernel/unit/memmap_test.cpp
  ${TEST}/net/unit/cookie_test.cpp
  ${TEST}/net/unit/ip4_addr.cpp
  ${TEST}/net/unit/tcp_packet_test.cpp
  ${TEST}/net/unit/tcp_socket.cpp
  ${TEST}/net/unit/tcp_write_buffer.cpp
  ${TEST}/net/unit/tcp_write_queue.cpp
  ${TEST}/posix/unit/fd_map_test.cpp
  ${TEST}/posix/unit/inet_test.cpp
  ${TEST}/util/unit/crc32.cpp
  ${TEST}/util/unit/delegate.cpp
  ${TEST}/util/unit/fixed_queue.cpp
  ${TEST}/util/unit/fixed_vector.cpp
  ${TEST}/util/unit/logger_test.cpp
  ${TEST}/util/unit/membitmap.cpp
  ${TEST}/util/unit/path_to_regex_no_options.cpp
  ${TEST}/util/unit/path_to_regex_parse.cpp
  ${TEST}/util/unit/path_to_regex_options.cpp
  ${TEST}/util/unit/ringbuffer.cpp
  ${TEST}/util/unit/statman.cpp
  ${TEST}/util/unit/uri_test.cpp
)

set(OS_SOURCES
  ${SRC}/fs/disk.cpp
  ${SRC}/fs/fat.cpp
  ${SRC}/fs/fat_async.cpp
  ${SRC}/fs/fat_sync.cpp
  ${SRC}/fs/filesystem.cpp
  ${SRC}/fs/mbr.cpp
  ${SRC}/fs/memdisk.cpp
  ${SRC}/fs/path.cpp
  ${SRC}/hw/block_device.cpp
  ${SRC}/hw/ioapic.cpp
  ${SRC}/hw/msi.cpp
  ${SRC}/hw/pci_device.cpp
  ${SRC}/hw/pci_msi.cpp
  ${SRC}/hw/pic.cpp
  ${SRC}/kernel/cpuid.cpp
  ${SRC}/kernel/main_call.cpp
  ${SRC}/kernel/memmap.cpp
  ${SRC}/net/http/cookie.cpp
  ${SRC}/net/util.cpp
  ${SRC}/posix/fd.cpp
  ${SRC}/posix/arpa/inet.cpp
  ${SRC}/util/crc32.cpp
  ${SRC}/util/logger.cpp
  ${SRC}/util/path_to_regex.cpp
  ${SRC}/util/percent_encoding.cpp
  ${SRC}/util/statman.cpp
  ${SRC}/util/uri.cpp
)

add_executable(unittests ${OS_SOURCES} ${TEST_SOURCES} )
install(TARGETS unittests DESTINATION ${TEST})
#add_custom_command(TARGET unittests POST_BUILD
#  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:unittests> ${TEST}
#  COMMENT "> Copying binary 'unittests' to ${TEST}")

install(DIRECTORY lest/include/lest DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

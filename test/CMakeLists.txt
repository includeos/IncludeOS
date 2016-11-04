cmake_minimum_required(VERSION 3.5)
project(unittests)

set(CMAKE_CXX_FLAGS "-m32 -std=c++14 -Dlest_FEATURE_AUTO_REGISTER=1 -DHAVE_LEST_MAIN")
set(SRC ${INCLUDEOS_ROOT}/src)
set(TEST ${INCLUDEOS_ROOT}/test)
set(INCLUDEOS_INSTALL $ENV{HOME}/IncludeOS_Install)

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
  #${TEST}/kernel/unit/memmap_test.cpp
  ${TEST}/net/unit/ip4_addr.cpp
  ${TEST}/net/unit/tcp_socket.cpp
  ${TEST}/net/unit/tcp_write_buffer.cpp
  ${TEST}/net/unit/tcp_write_queue.cpp
  ${TEST}/posix/unit/fd_map_test.cpp
  ${TEST}/util/unit/delegate.cpp
  ${TEST}/util/unit/fixed_vector.cpp
  ${TEST}/util/unit/ringbuffer.cpp
  #${TEST}/util/unit/statman.cpp
  ${TEST}/util/unit/membitmap.cpp
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
  ${SRC}/hw/drive.cpp
  ${SRC}/hw/ioapic.cpp
  ${SRC}/hw/msi.cpp
  ${SRC}/hw/pci_device.cpp
  ${SRC}/hw/pci_msi.cpp
  ${SRC}/hw/pic.cpp
  ${SRC}/kernel/cpuid.cpp
  ${SRC}/kernel/main_call.cpp
  ${SRC}/kernel/memmap.cpp
  ${SRC}/net/util.cpp
  ${SRC}/posix/fd.cpp
  ${SRC}/util/logger.cpp
  ${SRC}/util/statman.cpp
)

add_executable(unittests ${OS_SOURCES} ${TEST_SOURCES} )

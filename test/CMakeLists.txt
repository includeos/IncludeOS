cmake_minimum_required(VERSION 2.8.9)
project(unittests C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "-m32 -std=c11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS "-m32 -std=c++14 -Wall -Wextra -Wno-unused-function -D__id_t_defined -DUNITTESTS -DURI_THROW_ON_ERROR -DNO_INFO=1 -DNO_DEBUG=1 -DGSL_THROW_ON_CONTRACT_VIOLATION -Dlest_FEATURE_AUTO_REGISTER=1 -DHAVE_LEST_MAIN")

if(NOT DEFINED ${INCLUDEOS_ROOT})
  set(INCLUDEOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../)
endif()

set(SRC ${INCLUDEOS_ROOT}/src)
set(TEST ${INCLUDEOS_ROOT}/test)

include_directories(
  ${INCLUDEOS_ROOT}/api
  ${INCLUDEOS_ROOT}/api/posix
  ${INCLUDEOS_ROOT}/src/
  ${INCLUDEOS_ROOT}/src/include
  ${INCLUDEOS_ROOT}/mod/
  ${INCLUDEOS_ROOT}/mod/GSL
  ${INCLUDEOS_ROOT}/mod/uzlib/src
  ${TEST}/lest/include
  ${TEST}/lest_util
)

set(LEST_UTIL
  ${TEST}/lest_util/lestmain.cxx
  ${TEST}/lest_util/os_mock.cpp
)

set(TEST_SOURCES
  ${TEST}/fs/unit/block_device_test.cpp
  ${TEST}/fs/unit/memdisk_test.cpp
  ${TEST}/fs/unit/path_test.cpp
  ${TEST}/fs/unit/vfs_test.cpp
  ${TEST}/hw/unit/cpu_test.cpp
  ${TEST}/hw/unit/mac_addr_test.cpp
  ${TEST}/hw/unit/nic_test.cpp
  ${TEST}/kernel/unit/kprint_test.cpp
  ${TEST}/kernel/unit/memmap_test.cpp
  ${TEST}/kernel/unit/os_test.cpp
  ${TEST}/kernel/unit/rdrand_test.cpp
  ${TEST}/kernel/unit/service_stub_test.cpp
  ${TEST}/net/unit/cookie_test.cpp
  ${TEST}/net/unit/http_header_test.cpp
  ${TEST}/net/unit/http_status_codes_test.cpp
  ${TEST}/net/unit/http_method_test.cpp
  ${TEST}/net/unit/http_mime_types_test.cpp
  ${TEST}/net/unit/http_request_test.cpp
  ${TEST}/net/unit/http_response_test.cpp
  ${TEST}/net/unit/http_time_test.cpp
  ${TEST}/net/unit/http_version_test.cpp
  ${TEST}/net/unit/ip4_addr.cpp
  ${TEST}/net/unit/tcp_packet_test.cpp
  ${TEST}/net/unit/tcp_socket.cpp
  ${TEST}/net/unit/tcp_write_buffer.cpp
  ${TEST}/net/unit/tcp_write_queue.cpp
  ${TEST}/posix/unit/fd_map_test.cpp
  ${TEST}/posix/unit/inet_test.cpp
  ${TEST}/util/unit/chunk_test.cpp
  ${TEST}/util/unit/crc32.cpp
  ${TEST}/util/unit/delegate.cpp
  ${TEST}/util/unit/fixed_queue.cpp
  ${TEST}/util/unit/fixed_vector.cpp
  ${TEST}/util/unit/logger_test.cpp
  ${TEST}/util/unit/membitmap.cpp
  ${TEST}/util/unit/path_to_regex_no_options.cpp
  ${TEST}/util/unit/path_to_regex_parse.cpp
  ${TEST}/util/unit/path_to_regex_options.cpp
  ${TEST}/util/unit/percent_encoding_test.cpp
  ${TEST}/util/unit/ringbuffer.cpp
  ${TEST}/util/unit/statman.cpp
  ${TEST}/util/unit/uri_test.cpp
)

set(OS_SOURCES
  ${SRC}/boot/multiboot.cpp
  ${SRC}/fs/disk.cpp
  ${SRC}/fs/fat.cpp
  ${SRC}/fs/fat_async.cpp
  ${SRC}/fs/fat_sync.cpp
  ${SRC}/fs/filesystem.cpp
  ${SRC}/fs/memdisk.cpp
  ${SRC}/fs/mbr.cpp
  ${SRC}/fs/memdisk.cpp
  ${SRC}/fs/path.cpp
  ${SRC}/hw/acpi.cpp
  ${SRC}/hw/apic.cpp
  ${SRC}/hw/apic_revenant.cpp
  ${SRC}/hw/apic_timer.cpp
  ${SRC}/hw/block_device.cpp
  ${SRC}/hw/cpu_freq_sampling.cpp
  ${SRC}/hw/ioapic.cpp
  ${SRC}/hw/msi.cpp
  ${SRC}/hw/pci_device.cpp
  ${SRC}/hw/pci_msi.cpp
  ${SRC}/hw/pic.cpp
  ${SRC}/hw/pit.cpp
  ${SRC}/hw/ps2.cpp
  ${SRC}/hw/serial.cpp
  ${SRC}/kernel/block.cpp
  ${SRC}/kernel/cpuid.cpp
  ${SRC}/kernel/elf.cpp
  ${SRC}/kernel/irq_manager.cpp
  ${SRC}/kernel/kernel_start.cpp
  ${SRC}/kernel/main_call.cpp
  ${SRC}/kernel/memmap.cpp
  ${SRC}/kernel/os.cpp
  ${SRC}/kernel/pci_manager.cpp
  ${SRC}/kernel/rdrand.cpp
  ${SRC}/kernel/rng.cpp
  ${SRC}/kernel/service_stub.cpp
  ${SRC}/kernel/syscalls.cpp
  ${SRC}/kernel/terminal.cpp
  ${SRC}/kernel/terminal_disk.cpp
  ${SRC}/net/buffer_store.cpp
  ${SRC}/net/dns/client.cpp
  ${SRC}/net/dns/dns.cpp
  ${SRC}/net/ethernet/ethernet.cpp
  ${SRC}/net/http/client.cpp
  ${SRC}/net/http/connection.cpp
  ${SRC}/net/http/cookie.cpp
  ${SRC}/net/http/header.cpp
  ${SRC}/net/http/header_fields.cpp
  ${SRC}/net/http/message.cpp
  ${SRC}/net/http/mime_types.cpp
  ${SRC}/net/http/request.cpp
  ${SRC}/net/http/response.cpp
  ${SRC}/net/http/status_codes.cpp
  ${SRC}/net/http/time.cpp
  ${SRC}/net/http/version.cpp
  ${SRC}/net/inet_common.cpp
  ${SRC}/net/ip4/arp.cpp
  ${SRC}/net/ip4/icmpv4.cpp
  ${SRC}/net/ip4/ip4.cpp
  ${SRC}/net/ip4/udp.cpp
  ${SRC}/net/ip4/udp_socket.cpp
  ${SRC}/net/tcp/connection.cpp
  ${SRC}/net/tcp/connection_states.cpp
  ${SRC}/net/tcp/listener.cpp
  ${SRC}/net/tcp/rttm.cpp
  ${SRC}/net/tcp/tcp.cpp
  ${SRC}/net/tcp/write_queue.cpp
  ${SRC}/net/util.cpp
  ${SRC}/posix/fcntl.cpp
  ${SRC}/posix/fd.cpp
  ${SRC}/posix/sys/select.cpp
  ${SRC}/posix/arpa/inet.cpp
  ${SRC}/util/async.cpp
  ${SRC}/util/crc32.cpp
  ${SRC}/util/logger.cpp
  ${SRC}/util/path_to_regex.cpp
  ${SRC}/util/percent_encoding.cpp
  ${SRC}/util/statman.cpp
  ${SRC}/util/syslog_facility.cpp
  ${SRC}/util/syslogd.cpp
  ${SRC}/util/tar.cpp
  ${SRC}/util/uri.cpp
  ${SRC}/virtio/virtio_queue.cpp
)

set(MOD_OBJECTS
  ${INCLUDEOS_ROOT}/mod/http-parser/http_parser.c
)

# Only build selected sources with SINGLE
if(NOT SINGLE)
  set(SOURCES ${MOD_OBJECTS} ${OS_SOURCES} ${TEST_SOURCES} ${LEST_UTIL})
else()
  set(SOURCES ${SINGLE} ${LEST_UTIL})
endif()

add_executable(unittests ${SOURCES})
install(TARGETS unittests DESTINATION ${TEST})
install(DIRECTORY lest/include/lest DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

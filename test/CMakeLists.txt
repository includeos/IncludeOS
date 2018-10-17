cmake_minimum_required(VERSION 2.8.9)
project(unittests C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

option(COVERAGE "Build with coverage generation" OFF)
option(SILENT_BUILD "Build with some warnings turned off" ON)

option(INFO "Print INFO macro output" OFF)
option(DEBUG_INFO "Print debug macro output when DEBUG/DEBUG2 etc. is defined in source" OFF)
option(GENERATE_SUPPORT_FILES "Generate external files required by some tests (e.g. tar)" ON)
option(EXTRA_TESTS "Build extra test" OFF)

if ("${ARCH}" STREQUAL "")
  message(STATUS "CMake detected host arch: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
  set (ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})
endif("${ARCH}" STREQUAL "")
message(STATUS "Building for arch ${ARCH}")

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
add_definitions(-DPLATFORM_UNITTEST)
add_definitions(-DOS_VERSION="v0.0.0.1")

set(CMAKE_C_FLAGS "-g -O0 -std=c11 -Wall -Wextra")

set(NO_INFO "-DNO_INFO=1")
if(INFO)
  set(NO_INFO "")
endif()

set(NO_DEBUG "-DNO_DEBUG=1")
if (DEBUG_INFO)
  set(NO_DEBUG "")
endif()

set(CMAKE_CXX_FLAGS "-g -O0 -march=native -std=c++17 -Wall -Wextra -Wno-unused-function -D__id_t_defined -DUNITTESTS -DURI_THROW_ON_ERROR ${NO_INFO} ${NO_DEBUG} -DGSL_THROW_ON_CONTRACT_VIOLATION -Dlest_FEATURE_AUTO_REGISTER=1 -DHAVE_LEST_MAIN")

if (APPLE)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(STATUS "Including brew bundled libc++")
    execute_process(COMMAND brew "--prefix" "llvm@5" OUTPUT_VARIABLE BREW_LLVM)
    string(STRIP ${BREW_LLVM} BREW_LLVM)
    set(BREW_LIBCXX_INC "-L${BREW_LLVM}/lib -I${BREW_LLVM}/include/c++/v1")
    message(STATUS "Brew libc++ location: " ${BREW_LIBCXX_INC})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${BREW_LIBCXX_INC} -stdlib=libc++ -nostdinc++ -Wno-unused-command-line-argument")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmacosx-version-min=10.12")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.12")
  endif()
endif()

if(NOT DEFINED ${INCLUDEOS_ROOT})
  set(INCLUDEOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
endif()

set(SRC ${INCLUDEOS_ROOT}/src)
set(TEST ${INCLUDEOS_ROOT}/test)

include_directories(
  ${TEST}/lest_util
  ${INCLUDEOS_ROOT}/api
  ${INCLUDEOS_ROOT}/src/
  ${INCLUDEOS_ROOT}/src/include
  ${INCLUDEOS_ROOT}/mod/
  ${INCLUDEOS_ROOT}/mod/GSL
  ${INCLUDEOS_ROOT}/mod/uzlib/src
  ${INCLUDEOS_ROOT}/lib/LiveUpdate
  ${TEST}/lest/include
  $ENV{INCLUDEOS_PREFIX}/include
)

set(LEST_UTIL
  ${TEST}/lest_util/lestmain.cxx
  ${TEST}/lest_util/os_mock.cpp
  ${TEST}/lest_util/mock_fs.cpp
  ${TEST}/lest_util/random.cpp
)

set(TEST_SOURCES
  ${TEST}/fs/unit/block_device_test.cpp
  ${TEST}/fs/unit/memdisk_test.cpp
  ${TEST}/fs/unit/path_test.cpp
  ${TEST}/fs/unit/vfs_test.cpp
  ${TEST}/fs/unit/unit_fs.cpp
  ${TEST}/fs/unit/unit_fat.cpp
  #${TEST}/hw/unit/cpu_test.cpp
  ${TEST}/hw/unit/mac_addr_test.cpp
  ${TEST}/hw/unit/nic_test.cpp
  ${TEST}/kernel/unit/memmap_test.cpp
  ${TEST}/kernel/unit/memory.cpp
  ${TEST}/kernel/unit/x86_paging.cpp
  ${TEST}/kernel/unit/os_test.cpp
  ${TEST}/kernel/unit/service_stub_test.cpp
  ${TEST}/kernel/unit/unit_events.cpp
  ${TEST}/kernel/unit/unit_timers.cpp
  ${TEST}/kernel/unit/unit_liveupdate.cpp
  ${TEST}/net/unit/addr_test.cpp
  ${TEST}/net/unit/bufstore.cpp
  ${TEST}/net/unit/checksum.cpp
  ${TEST}/net/unit/cidr.cpp
  ${TEST}/net/unit/conntrack_test.cpp
  ${TEST}/net/unit/cookie_test.cpp
  ${TEST}/net/unit/dhcp_message_test.cpp
  ${TEST}/net/unit/error.cpp
  ${TEST}/net/unit/http_header_test.cpp
  ${TEST}/net/unit/http_status_codes_test.cpp
  ${TEST}/net/unit/http_method_test.cpp
  ${TEST}/net/unit/http_mime_types_test.cpp
  ${TEST}/net/unit/http_request_test.cpp
  ${TEST}/net/unit/http_response_test.cpp
  ${TEST}/net/unit/http_time_test.cpp
  ${TEST}/net/unit/http_version_test.cpp
  ${TEST}/net/unit/interfaces_test.cpp
  ${TEST}/net/unit/ip4_addr.cpp
  ${TEST}/net/unit/ip4.cpp
  ${TEST}/net/unit/ip4_packet_test.cpp
  ${TEST}/net/unit/ip6.cpp
  ${TEST}/net/unit/ip6_addr.cpp
  ${TEST}/net/unit/ip6_packet_test.cpp
  ${TEST}/net/unit/nat_test.cpp
  ${TEST}/net/unit/napt_test.cpp
  ${TEST}/net/unit/packets.cpp
  ${TEST}/net/unit/path_mtu_discovery.cpp
  ${TEST}/net/unit/port_util_test.cpp
  ${TEST}/net/unit/socket.cpp
  ${TEST}/net/unit/tcp_packet_test.cpp
  ${TEST}/net/unit/tcp_read_buffer_test.cpp
  ${TEST}/net/unit/tcp_read_request_test.cpp
  ${TEST}/net/unit/tcp_write_queue.cpp
  ${TEST}/net/unit/router_test.cpp
  ${TEST}/posix/unit/fd_map_test.cpp
  ${TEST}/posix/unit/inet_test.cpp
  ${TEST}/util/unit/base64.cpp
  ${TEST}/util/unit/sha1.cpp
  ${TEST}/util/unit/crc32.cpp
  ${TEST}/util/unit/delegate.cpp
  ${TEST}/util/unit/fixed_list_alloc_test.cpp
  ${TEST}/util/unit/fixed_queue.cpp
  ${TEST}/util/unit/fixed_vector.cpp
  ${TEST}/util/unit/isotime.cpp
  ${TEST}/util/unit/logger_test.cpp
  ${TEST}/util/unit/membitmap.cpp
  #${TEST}/util/unit/path_to_regex_no_options.cpp
  ${TEST}/util/unit/path_to_regex_parse.cpp
  ${TEST}/util/unit/path_to_regex_options.cpp
  ${TEST}/util/unit/percent_encoding_test.cpp
  ${TEST}/util/unit/ringbuffer.cpp
  ${TEST}/util/unit/statman.cpp
  ${TEST}/util/unit/syslogd_test.cpp
  ${TEST}/util/unit/syslog_facility_test.cpp
  ${TEST}/util/unit/uri_test.cpp
  ${TEST}/util/unit/bitops.cpp
  ${TEST}/util/unit/lstack.cpp
  ${TEST}/util/unit/buddy_alloc_test.cpp
)

set(OS_SOURCES
  ${SRC}/version.cpp
  ${SRC}/arch/x86_64/paging.cpp
  ${SRC}/fs/disk.cpp
  ${SRC}/fs/dirent.cpp
  ${SRC}/fs/fat.cpp
  ${SRC}/fs/fat_async.cpp
  ${SRC}/fs/fat_sync.cpp
  ${SRC}/fs/filesystem.cpp
  ${SRC}/fs/mbr.cpp
  ${SRC}/fs/memdisk.cpp
  ${SRC}/fs/path.cpp
  ${SRC}/hw/nic.cpp
  ${SRC}/hw/msi.cpp
  ${SRC}/hw/pci_device.cpp
  ${SRC}/hw/pci_msi.cpp
  ${SRC}/hw/ps2.cpp
  ${SRC}/kernel/cpuid.cpp
  ${SRC}/kernel/elf.cpp
  ${SRC}/kernel/events.cpp
  ${SRC}/kernel/memmap.cpp
  ${SRC}/kernel/os.cpp
  ${SRC}/kernel/pci_manager.cpp
  ${SRC}/kernel/rng.cpp
  ${SRC}/kernel/service_stub.cpp
  ${SRC}/kernel/syscalls.cpp
  ${SRC}/kernel/timers.cpp
  ${SRC}/musl/brk.cpp
  ${SRC}/net/addr.cpp
  ${SRC}/net/buffer_store.cpp
  ${SRC}/net/conntrack.cpp
  ${SRC}/net/dns/client.cpp
  ${SRC}/net/dns/dns.cpp
  ${SRC}/net/dns/query.cpp
  ${SRC}/net/dns/record.cpp
  ${SRC}/net/dns/response.cpp
  ${SRC}/net/ethernet/ethernet.cpp
  ${SRC}/net/http/basic_client.cpp
  ${SRC}/net/http/client_connection.cpp
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
  ${SRC}/net/checksum.cpp
  ${SRC}/net/inet.cpp
  ${SRC}/net/interfaces.cpp
  ${SRC}/net/ip4/arp.cpp
  ${SRC}/net/ip4/icmp4.cpp
  ${SRC}/net/ip4/ip4.cpp
  ${SRC}/net/ip4/reassembly.cpp
  ${SRC}/net/ip6/extension_header.cpp
  ${SRC}/net/ip6/icmp6.cpp
  ${SRC}/net/ip6/mld.cpp
  ${SRC}/net/ip6/ndp.cpp
  ${SRC}/net/ip6/packet_ndp.cpp
  ${SRC}/net/ip6/packet_mld.cpp
  ${SRC}/net/ip6/slaac.cpp
  ${SRC}/net/ip6/ip6.cpp
  ${SRC}/net/dhcp/dh4client.cpp
  ${SRC}/net/nat/nat.cpp
  ${SRC}/net/nat/napt.cpp
  ${SRC}/net/tcp/connection.cpp
  ${SRC}/net/tcp/connection_states.cpp
  ${SRC}/net/tcp/listener.cpp
  ${SRC}/net/tcp/rttm.cpp
  ${SRC}/net/tcp/tcp.cpp
  ${SRC}/net/tcp/read_buffer.cpp
  ${SRC}/net/tcp/read_request.cpp
  ${SRC}/net/tcp/stream.cpp
  ${SRC}/net/tcp/write_queue.cpp
  ${SRC}/net/udp/socket.cpp
  ${SRC}/net/udp/udp.cpp
  ${SRC}/posix/fd.cpp
  ${SRC}/util/async.cpp
  ${SRC}/util/crc32.cpp
  ${SRC}/util/logger.cpp
  ${SRC}/util/path_to_regex.cpp
  ${SRC}/util/percent_encoding.cpp
  ${SRC}/util/sha1.cpp
  ${SRC}/util/statman.cpp
  ${SRC}/util/syslog_facility.cpp
  ${SRC}/util/syslogd.cpp
  ${SRC}/util/tar.cpp
  ${SRC}/util/uri.cpp
  ${SRC}/virtio/virtio_queue.cpp
)

set(MOD_OBJECTS
  # http-parser
  ${INCLUDEOS_ROOT}/mod/http-parser/http_parser.c
  # uzlib
  ${INCLUDEOS_ROOT}/mod/uzlib/src/adler32.c
  ${INCLUDEOS_ROOT}/mod/uzlib/src/crc32.c
  ${INCLUDEOS_ROOT}/mod/uzlib/src/tinflate.c
  ${INCLUDEOS_ROOT}/mod/uzlib/src/tinfgzip.c
  # LiveUpdate
  #${INCLUDEOS_ROOT}/lib/LiveUpdate/hotswap.cpp
  ${INCLUDEOS_ROOT}/lib/LiveUpdate/partition.cpp
  ${INCLUDEOS_ROOT}/lib/LiveUpdate/rollback.cpp
  ${INCLUDEOS_ROOT}/lib/LiveUpdate/serialize_tcp.cpp
  ${INCLUDEOS_ROOT}/lib/LiveUpdate/update.cpp
  ${INCLUDEOS_ROOT}/lib/LiveUpdate/os.cpp
  ${INCLUDEOS_ROOT}/lib/LiveUpdate/resume.cpp
  #${INCLUDEOS_ROOT}/lib/LiveUpdate/serialize_openssl.cpp
  ${INCLUDEOS_ROOT}/lib/LiveUpdate/storage.cpp
)

# Disable (don't build) currently non-working tests on macOS
if (APPLE)
  list(REMOVE_ITEM TEST_SOURCES
    ${TEST}/kernel/unit/memory.cpp
    ${TEST}/kernel/unit/x86_paging.cpp
    ${TEST}/kernel/unit/unit_liveupdate.cpp
    )
endif()

if(EXTRA_TESTS)
  set(GENERATE_SUPPORT_FILES ON)
  message(STATUS "Adding some extra tests")
  list(APPEND TEST_SOURCES ${TEST}/kernel/unit/rdrand_test.cpp)
  list(APPEND TEST_SOURCES ${TEST}/util/unit/tar_test.cpp)

endif()

if(COVERAGE)
  message(STATUS "Coverage")
  list(REMOVE_ITEM TEST_SOURCES ${TEST}/util/unit/path_to_regex_no_options.cpp)
  set_property(SOURCE ${OS_SOURCES} PROPERTY COMPILE_FLAGS --coverage)
  set_property(SOURCE ${TEST_SOURCES} PROPERTY COMPILE_FLAGS --coverage)
  set_property(SOURCE ${MOD_OBJECTS} PROPERTY COMPILE_FLAGS --coverage)
  set_property(SOURCE ${LEST_UTIL} PROPERTY COMPILE_FLAGS --coverage)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

if(TRAVIS)
  message(STATUS "Travis")
  list(REMOVE_ITEM TEST_SOURCES ${TEST}/kernel/unit/rdrand_test.cpp)
endif()

if ("${ARCH}" STREQUAL "ARCH_ARMv7")
  list(REMOVE_ITEM TEST_SOURCES ${TEST}/hw/unit/cpu_test.cpp)
  list(REMOVE_ITEM OS_SOURCES ${SRC}/hw/ps2.cpp)
  list(REMOVE_ITEM OS_SOURCES ${SRC}/hw/serial.cpp)
  list(REMOVE_ITEM OS_SOURCES ${SRC}/kernel/cpuid.cpp)
  list(REMOVE_ITEM OS_SOURCES ${SRC}/kernel/irq_manager.cpp)
  list(REMOVE_ITEM OS_SOURCES ${SRC}/kernel/rdrand.cpp)
  list(REMOVE_ITEM OS_SOURCES ${SRC}/kernel/terminal.cpp)
endif("${ARCH}" STREQUAL "ARCH_ARMv7")

# Only build selected sources with SINGLE
if(NOT SINGLE)
  set(SOURCES ${MOD_OBJECTS} ${OS_SOURCES} ${TEST_SOURCES} ${LEST_UTIL})
else()
  set(SOURCES ${MOD_OBJECTS} ${OS_SOURCES} ${SINGLE} ${LEST_UTIL})
endif()

if ("${ARCH}" STREQUAL "ARCH_ARMv7")
  set_property(SOURCE ${SOURCES} PROPERTY COMPILE_FLAGS -mfpu=vfpv3-d16)
endif("${ARCH}" STREQUAL "ARCH_ARMv7")

if(SILENT_BUILD)
  message(STATUS "NOTE: Building with some warnings turned off")
  set_property(SOURCE ${SOURCES} APPEND_STRING PROPERTY COMPILE_FLAGS
  " -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-format")
endif()

add_executable(unittests ${SOURCES})
install(TARGETS unittests DESTINATION ${TEST})

if (GENERATE_SUPPORT_FILES)

  add_custom_command(
    TARGET unittests PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/test-single.tar ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/test-multiple.tar ${CMAKE_CURRENT_SOURCE_DIR}/*.py
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_BINARY_DIR}/test-invalid.tar
    COMMAND ${CMAKE_COMMAND} -E tar czf ${CMAKE_CURRENT_BINARY_DIR}/test.tar.gz ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E tar czf ${CMAKE_CURRENT_BINARY_DIR}/test-corrupt.gz ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    COMMAND bash ${TEST}/util/unit/corrupt-tar-gz.sh ${CMAKE_CURRENT_BINARY_DIR}/test.tar.gz ${CMAKE_CURRENT_BINARY_DIR}/test-corrupt.gz
    COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/test-tar-gz-inside.tar ${CMAKE_CURRENT_BINARY_DIR}/test.tar.gz
    )
endif()

add_custom_target(
  cppcheck
  COMMAND cppcheck
  --enable=warning,style,performance,portability
  --force
  --platform=unix32
  --std=c++11
  --verbose
  --quiet
  -I ${INCLUDEOS_ROOT}/api
  -I ${INCLUDEOS_ROOT}/src/include
  -I ${INCLUDEOS_ROOT}/mod/
  -I ${INCLUDEOS_ROOT}/mod/GSL
  -I ${INCLUDEOS_ROOT}/mod/uzlib/src
  ${OS_SOURCES}
)

add_custom_target(
  clang-tidy
  COMMAND clang-tidy-3.8
  -checks=clang-analyzer-core.*,clang-analyzer-cplusplus.*,clang-analyzer-deadcode.*,clang-analyzer-nullability,cppcoreguidelines*,modernize*,performance*,misc*,-misc-virtual-near-miss
  -p=compile_commands.json
  ${OS_SOURCES}
)

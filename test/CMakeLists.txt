cmake_minimum_required(VERSION 3.6.0)

project(unittests C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(PLATFORM "userspace")

if (NOT ARCH)
  set(ARCH "x86_64")
endif()
message(STATUS "Building for arch ${ARCH}")

option(INFO "Print INFO macro output" OFF)
option(DEBUG_INFO "Print debug macro output when DEBUG/DEBUG2 etc. is defined in source" OFF)
option(GENERATE_SUPPORT_FILES "Generate external files required by some tests (e.g. tar)" ON)
option(EXTRA_TESTS "Build extra test" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
add_definitions(-DPLATFORM_UNITTEST)

FILE(WRITE ${CMAKE_BINARY_DIR}/version.h
  "#define OS_VERSION \"v0.0.0.1\"\n"
)

#include_directories(${CMAKE_BINARY_DIR})

set(CMAKE_C_FLAGS "-g -O0 -std=c11 -Wall -Wextra")

set(NO_INFO "-DNO_INFO=1")
if(INFO)
  set(NO_INFO "")
endif()

set(NO_DEBUG "-DNO_DEBUG=1")
if (DEBUG_INFO)
  set(NO_DEBUG "")
endif()

set(CMAKE_CXX_FLAGS "-g -O0 -std=c++20 -Wall -Wextra -Wno-frame-address -Wno-unused-function -Wno-int-to-pointer-cast -D__id_t_defined -DUNITTESTS -DURI_THROW_ON_ERROR ${NO_INFO} ${NO_DEBUG} -DGSL_THROW_ON_CONTRACT_VIOLATION -Dlest_FEATURE_AUTO_REGISTER=1 -DHAVE_LEST_MAIN")

set(TEST ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
  ${TEST}/lest_util
  ${TEST}/../api
  ${TEST}/../src/include
  #TODO move to the right place
  ${TEST}/../lib/LiveUpdate/include
)

set(LEST_UTIL
  ${TEST}/lest_util/lestmain.cxx
  ${TEST}/lest_util/os_mock.cpp
  ${TEST}/lest_util/mock_fs.cpp
  ${TEST}/lest_util/random.cpp
)

set(TEST_SOURCES
  ${TEST}/fs/unit/memdisk_test.cpp
  ${TEST}/fs/unit/path_test.cpp
  ${TEST}/fs/unit/vfs_test.cpp
  ${TEST}/fs/unit/unit_fs.cpp
  ${TEST}/fs/unit/unit_fat.cpp
  #${TEST}/hw/unit/cpu_test.cpp
  ${TEST}/hw/unit/mac_addr_test.cpp
  ${TEST}/hw/unit/usernet.cpp
  ${TEST}/hw/unit/virtio_queue.cpp
  ${TEST}/kernel/unit/arch.cpp
  ${TEST}/kernel/unit/block.cpp
  ${TEST}/kernel/unit/cpuid.cpp
  ${TEST}/kernel/unit/memmap_test.cpp
  ${TEST}/kernel/unit/test_memory.cpp
  ${TEST}/kernel/unit/os_test.cpp
  ${TEST}/kernel/unit/rng.cpp
  ${TEST}/kernel/unit/service_stub_test.cpp
  ${TEST}/kernel/unit/test_hal.cpp
  ${TEST}/kernel/unit/unit_events.cpp
  ${TEST}/kernel/unit/unit_liveupdate.cpp
  ${TEST}/kernel/unit/unit_timers.cpp
  ${TEST}/kernel/unit/x86_paging.cpp
  ${TEST}/kernel/unit/spinlocks.cpp
  ${TEST}/net/unit/addr_test.cpp
  ${TEST}/net/unit/bufstore.cpp
  ${TEST}/net/unit/checksum.cpp
  ${TEST}/net/unit/cidr.cpp
  ${TEST}/net/unit/conntrack_test.cpp
  ${TEST}/net/unit/cookie_test.cpp
  ${TEST}/net/unit/dhcp.cpp
  ${TEST}/net/unit/dhcp_message_test.cpp
  ${TEST}/net/unit/error.cpp
  ${TEST}/net/unit/http_header_test.cpp
  ${TEST}/net/unit/http_status_codes_test.cpp
  ${TEST}/net/unit/http_method_test.cpp
  ${TEST}/net/unit/http_mime_types_test.cpp
#  ${TEST}/net/unit/http_request_test.cpp
#  ${TEST}/net/unit/http_response_test.cpp
  ${TEST}/net/unit/http_time_test.cpp
  ${TEST}/net/unit/http_version_test.cpp
  ${TEST}/net/unit/interfaces_test.cpp
  ${TEST}/net/unit/ip4_addr.cpp
  ${TEST}/net/unit/ip4.cpp
  ${TEST}/net/unit/ip4_packet_test.cpp
  ${TEST}/net/unit/ip6.cpp
  ${TEST}/net/unit/ip6_addr.cpp
  ${TEST}/net/unit/ip6_addr_list_test.cpp
  ${TEST}/net/unit/ip6_packet_test.cpp
  ${TEST}/net/unit/nat_test.cpp
  ${TEST}/net/unit/napt_test.cpp
  ${TEST}/net/unit/packets.cpp
  ${TEST}/net/unit/path_mtu_discovery.cpp
  ${TEST}/net/unit/port_util_test.cpp
  ${TEST}/net/unit/router_test.cpp
  ${TEST}/net/unit/socket.cpp
  ${TEST}/net/unit/stateful_addr_test.cpp
  ${TEST}/net/unit/tcp_benchmark.cpp
  ${TEST}/net/unit/tcp_packet_test.cpp
  ${TEST}/net/unit/tcp_read_buffer_test.cpp
  ${TEST}/net/unit/tcp_read_request_test.cpp
  ${TEST}/net/unit/tcp_write_queue.cpp
#  ${TEST}/net/unit/websocket.cpp
  ${TEST}/posix/unit/fd_map_test.cpp
  ${TEST}/posix/unit/inet_test.cpp
  ${TEST}/posix/unit/unit_fd.cpp
  ${TEST}/util/unit/base64.cpp
  ${TEST}/util/unit/bitops.cpp
  ${TEST}/util/unit/buddy_alloc_test.cpp
  ${TEST}/util/unit/config.cpp
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
  ${TEST}/util/unit/pmr_alloc_test.cpp
  ${TEST}/util/unit/ringbuffer.cpp
  ${TEST}/util/unit/sha1.cpp
  ${TEST}/util/unit/statman.cpp
  ${TEST}/util/unit/syslogd_test.cpp
  ${TEST}/util/unit/syslog_facility_test.cpp
#  ${TEST}/util/unit/uri_test.cpp
  ${TEST}/util/unit/lstack/test_lstack_nodes.cpp
  ${TEST}/util/unit/lstack/test_lstack_merging.cpp
  ${TEST}/util/unit/lstack/test_lstack_nomerge.cpp
)

if(EXTRA_TESTS)
  set(GENERATE_SUPPORT_FILES ON)
  message(STATUS "Adding some extra tests")
  list(APPEND TEST_SOURCES ${TEST}/util/unit/tar_test.cpp)
endif()

enable_testing()

if (CPPCHECK)
  find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
  if (NOT CMAKE_CXX_CPPCHECK)
    message(WARNING "cppcheck not found")
  else()
      list(
          APPEND CMAKE_CXX_CPPCHECK
              "--enable=warning"
              "--inconclusive"
              "--force"
              "--inline-suppr"
            #TODO   "--suppressions-list=${CMAKE_SOURCE_DIR}/CppCheckSuppressions.txt"
      )
  endif()
endif()

add_subdirectory(../src os)
add_subdirectory(../lib/LiveUpdate liveupdate)
add_library(lest_util ${LEST_UTIL})

file(COPY memdisk.fat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

find_program( VALGRIND valgrind )

SET(TEST_BINARIES)
foreach(T ${TEST_SOURCES})
  #CTest style
  #get the filename witout extension
  get_filename_component(NAME ${T} NAME_WE)
  add_executable(${NAME} ${T})
  target_link_libraries(${NAME} liveupdate os lest_util os m stdc++)

  # regular test
  add_test(${NAME}_unit bin/${NAME})
  set_property(TEST ${NAME}_unit
    PROPERTY LABELS unit)

  # valgrind / memcheck
  add_test(${NAME}_memcheck ${VALGRIND}
      --error-exitcode=1
      -s
      --track-origins=yes
      --tool=memcheck
      bin/${NAME})

  set_property(TEST ${NAME}_memcheck
    PROPERTY LABELS memcheck)

  # clang-tidy (slow...)
  add_test(${NAME}_clangtidy clang-tidy
      -checks=clang-analyzer-core.*,clang-analyzer-cplusplus.*,clang-analyzer-deadcode.*,clang-analyzer-nullability,cppcoreguidelines*,modernize*,performance*,misc*,-misc-virtual-near-miss
      -p=compile_commands.json
      ${T})

  set_property(TEST ${NAME}_clangtidy
    PROPERTY LABELS clangtidy)

  list(APPEND TEST_BINARIES ${NAME})
endforeach()

add_custom_target( unittests ALL
  DEPENDS ${TEST_BINARIES})

add_custom_command(TARGET unittests
                   POST_BUILD
                   COMMAND ctest --output-on-failure -L unit --timeout 5)

add_custom_target( memcheck
  DEPENDS ${TEST_BINARIES})

add_custom_command(TARGET memcheck
                   POST_BUILD
                   COMMAND ctest --output-on-failure -L memcheck --timeout 5)

add_custom_target( clangtidy
  DEPENDS ${TEST_BINARIES})

add_custom_command(TARGET clangtidy
                   POST_BUILD
                   COMMAND ctest --output-on-failure -L clangtidy)

if (GENERATE_SUPPORT_FILES)
  add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test-tar-gz-inside.tar
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/test-single.tar ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/test-multiple.tar ${CMAKE_CURRENT_SOURCE_DIR}/*.py
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_BINARY_DIR}/test-invalid.tar
    COMMAND ${CMAKE_COMMAND} -E tar czf ${CMAKE_CURRENT_BINARY_DIR}/test.tar.gz ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E tar czf ${CMAKE_CURRENT_BINARY_DIR}/test-corrupt.gz ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    COMMAND bash ${TEST}/util/unit/corrupt-tar-gz.sh ${CMAKE_CURRENT_BINARY_DIR}/test.tar.gz ${CMAKE_CURRENT_BINARY_DIR}/test-corrupt.gz
    COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/test-tar-gz-inside.tar ${CMAKE_CURRENT_BINARY_DIR}/test.tar.gz
    )
endif()

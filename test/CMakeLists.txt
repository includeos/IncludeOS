cmake_minimum_required(VERSION 3.31.6)

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

set(CMAKE_CXX_FLAGS "-g -O0 -std=c++20 -Wall -Wextra -Wno-frame-address -Wno-unused-function -Wno-int-to-pointer-cast -D__id_t_defined -DUNITTESTS -DURI_THROW_ON_ERROR ${NO_INFO} ${NO_DEBUG} -DGSL_THROW_ON_CONTRACT_VIOLATION -Dlest_FEATURE_AUTO_REGISTER=1 -DHAVE_LEST_MAIN -DPAGE_SIZE=4096")

set(TESTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(TESTS_DEPS ${TESTS_ROOT}/misc)
set(UNIT_TESTS ${TESTS_ROOT}/tests-unit)
set(INCLUDEOS  ${TESTS_ROOT}/..)

include_directories(
  ${TESTS_DEPS}/lest_util
  ${INCLUDEOS}/api
  ${INCLUDEOS}/src/include
  #TODO move to the right place
  ${INCLUDEOS}/lib/LiveUpdate/include
)

set(LEST_UTIL
  ${TESTS_DEPS}/lest_util/lestmain.cxx
  ${TESTS_DEPS}/lest_util/os_mock.cpp
  ${TESTS_DEPS}/lest_util/mock_fs.cpp
  ${TESTS_DEPS}/lest_util/random.cpp
)

# TODO: maybe just use `*.cpp *.hpp` globs here?
set(TEST_SOURCES
  ${UNIT_TESTS}/fs/memdisk_test.cpp
  ${UNIT_TESTS}/fs/path_test.cpp
  ${UNIT_TESTS}/fs/vfs_test.cpp
  ${UNIT_TESTS}/fs/unit_fs.cpp
  ${UNIT_TESTS}/fs/unit_fat.cpp
# ${UNIT_TESTS}/hw/cpu_test.cpp
  ${UNIT_TESTS}/hw/mac_addr_test.cpp
  ${UNIT_TESTS}/hw/usernet.cpp
  ${UNIT_TESTS}/hw/virtio_queue.cpp
  ${UNIT_TESTS}/kernel/arch.cpp
  ${UNIT_TESTS}/kernel/blocking.cpp
  ${UNIT_TESTS}/kernel/cpuid.cpp
  ${UNIT_TESTS}/kernel/memmap_test.cpp
  ${UNIT_TESTS}/kernel/test_memory.cpp
  ${UNIT_TESTS}/kernel/os_test.cpp
  ${UNIT_TESTS}/kernel/rng.cpp
  ${UNIT_TESTS}/kernel/service_stub_test.cpp
  ${UNIT_TESTS}/kernel/test_hal.cpp
  ${UNIT_TESTS}/kernel/unit_events.cpp
  ${UNIT_TESTS}/kernel/unit_liveupdate.cpp
  ${UNIT_TESTS}/kernel/unit_timers.cpp
  ${UNIT_TESTS}/kernel/x86_paging.cpp
  ${UNIT_TESTS}/kernel/spinlocks.cpp
  ${UNIT_TESTS}/net/addr_test.cpp
  ${UNIT_TESTS}/net/bufstore.cpp
  ${UNIT_TESTS}/net/checksum.cpp
  ${UNIT_TESTS}/net/cidr.cpp
  ${UNIT_TESTS}/net/conntrack_test.cpp
  ${UNIT_TESTS}/net/cookie_test.cpp
  ${UNIT_TESTS}/net/dhcp.cpp
  ${UNIT_TESTS}/net/dhcp_message_test.cpp
  ${UNIT_TESTS}/net/error.cpp
  ${UNIT_TESTS}/net/http_header_test.cpp
  ${UNIT_TESTS}/net/http_status_codes_test.cpp
  ${UNIT_TESTS}/net/http_method_test.cpp
  ${UNIT_TESTS}/net/http_mime_types_test.cpp
# ${UNIT_TESTS}/net/http_request_test.cpp
# ${UNIT_TESTS}/net/http_response_test.cpp
  ${UNIT_TESTS}/net/http_time_test.cpp
  ${UNIT_TESTS}/net/http_version_test.cpp
  ${UNIT_TESTS}/net/interfaces_test.cpp
  ${UNIT_TESTS}/net/ip4_addr.cpp
  ${UNIT_TESTS}/net/ip4.cpp
  ${UNIT_TESTS}/net/ip4_packet_test.cpp
  ${UNIT_TESTS}/net/ip6.cpp
  ${UNIT_TESTS}/net/ip6_addr.cpp
  ${UNIT_TESTS}/net/ip6_addr_list_test.cpp
  ${UNIT_TESTS}/net/ip6_packet_test.cpp
  ${UNIT_TESTS}/net/nat_test.cpp
  ${UNIT_TESTS}/net/napt_test.cpp
  ${UNIT_TESTS}/net/packets.cpp
  ${UNIT_TESTS}/net/path_mtu_discovery.cpp
  ${UNIT_TESTS}/net/port_util_test.cpp
  ${UNIT_TESTS}/net/router_test.cpp
  ${UNIT_TESTS}/net/socket.cpp
  ${UNIT_TESTS}/net/stateful_addr_test.cpp
  ${UNIT_TESTS}/net/tcp_benchmark.cpp
  ${UNIT_TESTS}/net/tcp_packet_test.cpp
  ${UNIT_TESTS}/net/tcp_read_buffer_test.cpp
  ${UNIT_TESTS}/net/tcp_read_request_test.cpp
  ${UNIT_TESTS}/net/tcp_write_queue.cpp
# ${UNIT_TESTS}/net/websocket.cpp
  ${UNIT_TESTS}/posix/fd_map_test.cpp
  ${UNIT_TESTS}/posix/inet_test.cpp
  ${UNIT_TESTS}/posix/unit_fd.cpp
  ${UNIT_TESTS}/util/base64.cpp
  ${UNIT_TESTS}/util/bitops.cpp
  ${UNIT_TESTS}/util/buddy_alloc_test.cpp
  ${UNIT_TESTS}/util/config.cpp
  ${UNIT_TESTS}/util/crc32.cpp
  ${UNIT_TESTS}/util/delegate.cpp
  ${UNIT_TESTS}/util/fixed_list_alloc_test.cpp
  ${UNIT_TESTS}/util/fixed_queue.cpp
  ${UNIT_TESTS}/util/fixed_vector.cpp
  ${UNIT_TESTS}/util/isotime.cpp
  ${UNIT_TESTS}/util/logger_test.cpp
  ${UNIT_TESTS}/util/membitmap.cpp
# ${UNIT_TESTS}/util/path_to_regex_no_options.cpp
  ${UNIT_TESTS}/util/path_to_regex_parse.cpp
  ${UNIT_TESTS}/util/path_to_regex_options.cpp
  ${UNIT_TESTS}/util/percent_encoding_test.cpp
  ${UNIT_TESTS}/util/pmr_alloc_test.cpp
  ${UNIT_TESTS}/util/ringbuffer.cpp
  ${UNIT_TESTS}/util/sha1.cpp
  ${UNIT_TESTS}/util/statman.cpp
  ${UNIT_TESTS}/util/syslogd_test.cpp
  ${UNIT_TESTS}/util/syslog_facility_test.cpp
# ${UNIT_TESTS}/util/uri_test.cpp
  ${UNIT_TESTS}/util/lstack/test_lstack_nodes.cpp
  ${UNIT_TESTS}/util/lstack/test_lstack_merging.cpp
  ${UNIT_TESTS}/util/lstack/test_lstack_nomerge.cpp
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

file(COPY ${UNIT_TESTS}/fs/memdisk.fat DESTINATION ${CMAKE_CURRENT_BINARY_DIR})  # TODO: handle special files within each unit test

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
    COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/test-single.tar ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/test-multiple.tar ${CMAKE_CURRENT_SOURCE_DIR}/*.py
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_BINARY_DIR}/test-invalid.tar
    COMMAND ${CMAKE_COMMAND} -E tar czf ${CMAKE_CURRENT_BINARY_DIR}/test.tar.gz ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E tar czf ${CMAKE_CURRENT_BINARY_DIR}/test-corrupt.gz ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    COMMAND bash ${UNIT_TESTS}/util/corrupt-tar-gz.sh ${CMAKE_CURRENT_BINARY_DIR}/test.tar.gz ${CMAKE_CURRENT_BINARY_DIR}/test-corrupt.gz
    COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/test-tar-gz-inside.tar ${CMAKE_CURRENT_BINARY_DIR}/test.tar.gz
    )
endif()

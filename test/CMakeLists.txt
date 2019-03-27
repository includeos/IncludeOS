cmake_minimum_required(VERSION 3.1.0)

project(unittests C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(PLATFORM "userspace")

if (EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
else()
  #TODO get a released version ? or put one into root/cmake/ ?
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
     message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
     file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                    "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()
  ##needed by conaningans

  include(${CMAKE_BINARY_DIR}/conan.cmake)
  #include conan cmake
  if (UPDATE)
   set(CONAN_UPDATE UPDATE)
  endif()
  if (CONAN_PROFILE)
    set(CONANPROFILE PROFILED ${CONAN_PROFILE})
  endif()
  conan_cmake_run(
    CONANFILE conanfile.txt
    BASIC_SETUP
    ${CONAN_UPDATE}
    ${CONANPROFILE}
  )
endif()

if (NOT ARCH)
  if (CONAN_SETTINGS_ARCH)
    set(ARCH ${CONAN_SETTINGS_ARCH})
  else()
    set(ARCH "x86_64")
  endif()
endif()
message(STATUS "Building for arch ${ARCH}")

option(COVERAGE "Build with coverage generation" OFF)
option(SILENT_BUILD "Build with some warnings turned off" ON)

option(INFO "Print INFO macro output" OFF)
option(DEBUG_INFO "Print debug macro output when DEBUG/DEBUG2 etc. is defined in source" OFF)
option(GENERATE_SUPPORT_FILES "Generate external files required by some tests (e.g. tar)" ON)
option(EXTRA_TESTS "Build extra test" OFF)

#if ("${ARCH}" STREQUAL "")
#  message(STATUS "CMake detected host arch: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
#  set (ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})
#endif("${ARCH}" STREQUAL "")

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
add_definitions(-DPLATFORM_UNITTEST)

FILE(WRITE ${CMAKE_BINARY_DIR}/version.h
  "#define OS_VERSION \"v0.0.0.1\"\n"
)

include_directories(${CMAKE_BINARY_DIR})

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

if (COVERAGE)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  endif()
endif()

# Neccesary apple stuff because native bundle is old/bad
if (APPLE)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    message(STATUS "Including brew bundled libc++")
    execute_process(COMMAND brew "--prefix" "llvm@6" OUTPUT_VARIABLE BREW_LLVM)
    string(STRIP ${BREW_LLVM} BREW_LLVM)
    set(BREW_LIBCXX_INC "-L${BREW_LLVM}/lib -I${BREW_LLVM}/include/c++/v1")
    message(STATUS "Brew libc++ location: " ${BREW_LIBCXX_INC})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${BREW_LIBCXX_INC} -stdlib=libc++ -nostdinc++ -lc++experimental -Wno-unused-command-line-argument")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmacosx-version-min=10.12")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.12")
  endif()
endif()

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
  ${TEST}/kernel/unit/memory.cpp
  ${TEST}/kernel/unit/os_test.cpp
  ${TEST}/kernel/unit/rng.cpp
  ${TEST}/kernel/unit/service_stub_test.cpp
  ${TEST}/kernel/unit/test_hal.cpp
  ${TEST}/kernel/unit/unit_events.cpp
  ${TEST}/kernel/unit/unit_liveupdate.cpp
  ${TEST}/kernel/unit/unit_timers.cpp
  ${TEST}/kernel/unit/x86_paging.cpp
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
  ${TEST}/net/unit/websocket.cpp
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
  ${TEST}/util/unit/uri_test.cpp
  ${TEST}/util/unit/lstack/test_lstack_nodes.cpp
  ${TEST}/util/unit/lstack/test_lstack_merging.cpp
  ${TEST}/util/unit/lstack/test_lstack_nomerge.cpp
)

# Disable (don't build) currently non-working tests on macOS
if (APPLE)
  list(REMOVE_ITEM TEST_SOURCES
    ${TEST}/net/unit/websocket.cpp
    )
endif()

if (COVERAGE)
  list(REMOVE_ITEM TEST_SOURCES ${TEST}/util/unit/path_to_regex_no_options.cpp)
endif()

if(EXTRA_TESTS)
  set(GENERATE_SUPPORT_FILES ON)
  message(STATUS "Adding some extra tests")
  list(APPEND TEST_SOURCES ${TEST}/util/unit/tar_test.cpp)
endif()

#TODO get from conan!!! or should the test be in liveupdate and not vise versa?

#if we could do add directory here it would be a lot cleaner..
#TODO investigate

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

SET(TEST_BINARIES)
foreach(T ${TEST_SOURCES})
  #CTest style
  #get the filename witout extension
  get_filename_component(NAME ${T} NAME_WE)
  add_executable(${NAME} ${T})
  target_link_libraries(${NAME} liveupdate os lest_util os m stdc++ ${CONAN_LIB_DIRS_HTTP-PARSER}/http_parser.o)
  add_test(${NAME} bin/${NAME})
  #add to list of tests for dependencies
  list(APPEND TEST_BINARIES ${NAME})
endforeach()

if(SILENT_BUILD)
  message(STATUS "NOTE: Building with some warnings turned off")
  set_property(SOURCE ${SOURCES} APPEND_STRING PROPERTY COMPILE_FLAGS
  " -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-format")
endif()

#disabled old DUNITTESTS
if (COVERAGE)
  if (NOT DEFINED CODECOV_OUTPUTFILE )
      set( CODECOV_OUTPUTFILE coverage.info )
  endif()

  if (NOT DEFINED CODECOV_HTMLOUTPUTDIR )
      set( CODECOV_HTMLOUTPUTDIR ${CMAKE_CURRENT_BINARY_DIR}/html )
  endif()
  if (CMAKE_COMPILER_IS_GNUCXX)
    if (NOT DEFINED CODECOV_GCOV)
      find_program(CODECOV_GCOV gcov)
    endif()
    if (NOT DEFINED CODECOV_LCOV)
      find_program(CODECOV_LCOV lcov)
    endif()
    if (NOT DEFINED CODECOV_GENHTML)
      find_program(CODECOV_GENHTML genhtml)
    endif()

    #run this command allways althoug it depends on all the sources beeing buildt
    add_custom_target(coverage_init ALL
      COMMENT "Generating empty initial coverage"
      COMMAND ${CODECOV_LCOV} -q --gcov-tool ${CODECOV_GCOV} -b ${CMAKE_CURRENT_BINARY_DIR} -d ${CMAKE_CURRENT_BINARY_DIR} -o base.info -c -i
      DEPENDS ${TEST_BINARIES}
      BYPRODUCTS base.info
    )
    add_custom_target(execute_test
      COMMENT "Executing test"
      COMMAND ctest
      DEPENDS ${TEST_BINARIES}
    )

    #generate coverage for the excuted code.. TODO make it run all the code ?
    add_custom_target(coverage_executed
      COMMENT "Generating executed coverage"
      COMMAND ${CODECOV_LCOV} -q --gcov-tool ${CODECOV_GCOV} -b ${CMAKE_CURRENT_BINARY_DIR} -d ${CMAKE_CURRENT_BINARY_DIR} -o test.info -c
      BYPRODUCTS test.info
      DEPENDS execute_test
    )
    #merge generated coverage and executed coverage.
    add_custom_target(coverage_merged
      COMMENT "Merging initial and executed coverage"
      COMMAND ${CODECOV_LCOV} -q --gcov-tool ${CODECOV_GCOV} -a base.info -a test.info -o unstripped.info
      SOURCES test.info base.info
      BYPRODUCTS unstripped.info
      DEPENDS coverage_init coverage_executed
    )

    #strip from report /test path /usr/lib and /usr/include.. TODO add more if neccesary eg */.conan/*
    add_custom_target(coverage_stripped
      COMMENT "Stripping default paths and test from report"
      COMMAND ${CODECOV_LCOV} -q -r unstripped.info '*/test/*' '/usr/lib/*' '/usr/include/*'  -o ${CODECOV_OUTPUTFILE}
      DEPENDS coverage_merged
      SOURCES unstripped.info
      BYPRODUCTS ${CODECOV_OUTPUTFILE}
    )
    #generate HTML coverage report
    add_custom_target(coverage
      COMMENT "Generating Code Coverage Web report"
      COMMAND ${CODECOV_GENHTML} -q -o ${CODECOV_HTMLOUTPUTDIR} ${CODECOV_OUTPUTFILE}
      DEPENDS coverage_stripped
      SOURCES ${CODECOV_OUTPUTFILE}
    )
  endif()
endif()

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

add_custom_target(
  clang-tidy
  COMMAND clang-tidy-3.8
  -checks=clang-analyzer-core.*,clang-analyzer-cplusplus.*,clang-analyzer-deadcode.*,clang-analyzer-nullability,cppcoreguidelines*,modernize*,performance*,misc*,-misc-virtual-near-miss
  -p=compile_commands.json
)

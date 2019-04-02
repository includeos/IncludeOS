cmake_minimum_required(VERSION 3.10)

option(STRESS "Enable includeos stress test" OFF)
project(IntegrationTests)

#TODO #INCLUDEOS_PREFIX.-. (NOT NEEDED IF CONAN?) but maybe check it?

#TODO check these and notify if any is missing
find_program(HPING3 hping3)
find_program(NODEJS nodejs node) #if not found look for node

if (HPING3-NOTFOUND)
  set(FAIL True)
  message("missing test dependency hping3 some tests will fail")
endif()
if (NODEJS-NOTFOUND)
  set(FAIL True)
  message("missing test dependency nodejs, node some tests will fail")
endif()


if (NOT NO_FAIL AND FAIL)
  message(FATAL_ERROR "Integration test requirements are not met see previous message")
endif()


#TODO ADD ws4py to deps
enable_testing()

SET(TEST_PARAMS 3)
SET(NAME_PARAM_INDEX    0)
SET(TIMEOUT_PARAM_INDEX 1)
SET(TYPE_PARAM_INDEX    2)
SET(FALLBACK_TIMEOUT  400)

#you get the idea ..
#testname = FOLDER + EXECUTABLE
#timeout how long should the test execute before cmake screems
#type what type of test is this (also to deduct subfolder)
set(TEST_LIST
  #FS test
  "fat16"     "20" "fs"
  "fat32"     "30" "fs"
#TODO REMOVE IDE
# "ide"       "30" "fs"
#  "ide_write" "20" "fs"
  "memdisk"   "20" "fs"
  "vfs"       "20" "fs"
  "virtio_block"  "30" "fs"

  #HW tests todo ?
  #"serial" "20" "hw"
  #"vga" "20" "hw"
  #"virtio_queue" "20" "hw"

  "gsl" "20" "mod"

  #plugin
  "unik" "20" "plugin"

  #posix
  "conf"    "20" "posix"
  "file_fd" "20" "posix"
  "main"    "20" "posix"
  #syscall error
  #"pthread" "20" "posix"
  "stat"    "20" "posix"
  "syslog_default" "20" "posix"
  "syslog_plugin" "20" "posix"

  # Disable posix_tcp as it is highly unreliable
  #"tcp"   "10"  "posix"
  "udp"   "20"  "posix"
  "utsname" "20" "posix"

  #stl
  "exceptions" "20" "stl"
  "crt" "20" "stl"
  "stl" "20" "stl"
  "coroutines" "20" "stl"

  #util
  "tar_gz" "20" "util"
  "tar" "20" "util"

  #NET tests
  "bufstore"  "30"    "net"
  "configure" "30"    "net"
  "dns"       "20"    "net"
  #TODO FIX
  #gateway disabled as it needs nacl.. circular DEP issue
  #"gateway"   "50"    "net"
  #TODO move http to http library
  #"http"      "20"    "net"
  "icmp"      "50"    "net"
  "icmp6"     "50"    "net"
  #TODO move to microlb with framework to test
  #"microLB"   "50"    "net"
  "nat"       "30"    "net"
  "router"    "30"    "net"
  "router6"   "30"    "net"
  "slaac"     "30"    "net"
  "tcp"       "120"   "net"
  "udp"       "30"    "net"
  "vlan"      "20"    "net"
  #TODO move websocket to http library
  "websocket" "20"    "net"

  #TODO add a cmake variable to exclude these ?
  "dhclient" "20" "net"
  "dhcpd" "20" "net"
  #dhcpd_dhclient_linux

  "block"     "40" "kernel"
  ##TODO check if context test is old and should be removed!!
  #"context"   "20" "kernel"
  "exception" "20" "kernel"
  #"fiber"     "20" "kernel"
  "grub"  "20" "kernel"
  "kprint"    "10" "kernel"
  "LiveUpdate" "10" "kernel"
  "memmap"    "20" "kernel"
  #This failes on jenkins but not locally trying to disable it to verify that tests run
  #"modules" "20" "kernel"
  "paging"    "20" "kernel"
  "plugin_init" "60" "kernel"
  "rng"       "20" "kernel"
  "smp"       "20" "kernel"
  "term"      "40" "kernel"
  "timers"    "60" "kernel"
  "tls"       "20" "kernel"
)

function(get_list_param LIST item_index INDEX PARAM)
  math(EXPR param_index '${item_index}+${INDEX}')
  list(GET LIST ${param_index} VALUE)
  set(${PARAM} ${VALUE} PARENT_SCOPE)
endfunction()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

#if not run already run conan install to get dependencies sorted
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  if (CONAN_PROFILE)
    set(CONANPROFILE PROFILE ${CONAN_PROFILE})
  endif()
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
     message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
     file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.13/conan.cmake"
                   "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()

  include(${CMAKE_BINARY_DIR}/conan.cmake)
  conan_cmake_run(
    CONANFILE conanfile.txt

    #PENDING PR to conan-cmake
    #NO_LOAD
  #  INSTALL_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}
    ${CONANPROFILE}
  )
endif()

function(do_conaningans TARGET SOURCE)
  #this package needs special care..
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
     message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
     file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.13/conan.cmake"
                   "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()

  include(${CMAKE_BINARY_DIR}/conan.cmake)

  set(CONAN_FILE ${SOURCE}/conanfile.txt)

  set(OLD_DIR ${CMAKE_CURRENT_BINARY_DIR})
  #hack for conan_cmake_run
  set(CMAKE_CURRENT_BINARY_DIR ${OLD_DIR}/${TARGET})
  file(MAKE_DIRECTORY ${OLD_DIR}/${TARGET})
  #clear any old libs from last pass.. this is a ugly hack i wish it was an option to cmake_run
  set(CONAN_LIBS "")
  conan_cmake_run(
    CONANFILE ${SOURCE}/conanfile.txt

      #PENDING PR to conan-cmake
      #NO_LOAD
    #  INSTALL_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}
    ${CONANPROFILE}
  )
  set(CMAKE_CURRENT_BINARY_DIR ${OLD_DIR})
endfunction()

function(add_integration_tests TESTS)
  list(LENGTH TESTS LISTLEN)
  math(EXPR LEN_MINUS_1 '${LISTLEN}-1')
  foreach(INDEX RANGE 0 ${LEN_MINUS_1} ${TEST_PARAMS})
    get_list_param("${TESTS}" ${INDEX} ${NAME_PARAM_INDEX} T)
    get_list_param("${TESTS}" ${INDEX} ${TIMEOUT_PARAM_INDEX} TIMEOUT)
    get_list_param("${TESTS}" ${INDEX} ${TYPE_PARAM_INDEX} TYPE)

    message(STATUS "Adding test integration_${TYPE}_${T} with timeout ${TIMEOUT}")
    #if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}/conanfile.txt)
    #  do_conaningans(${TYPE}/${T} "../${TYPE}/integration/${T}")
  #endif()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../${TYPE}/integration/${T}  ${TYPE}/${T})

    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../${TYPE}/integration/${T}/test.py)
      add_test(NAME integration_${TYPE}_${T}
        #TODO make cmake find python version .. and update vmrunner
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh ${CMAKE_BINARY_DIR} ${TIMEOUT} ${CMAKE_CURRENT_BINARY_DIR}/${TYPE}/${T}/${TYPE}_${T}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${TYPE}/${T}
      )
      set_property(TEST integration_${TYPE}_${T} PROPERTY TIMEOUT_AFTER_MATCH "0.1" "Test timed out")
      set_property(TEST integration_${TYPE}_${T} PROPERTY TIMEOUT ${FALLBACK_TIMEOUT})
    else()
      message(WARNING "No test.py present in ${CMAKE_CURRENT_SOURCE_DIR}/../${TYPE}/integration/${T}")
    endif()
  endforeach()
endfunction()

if (STRESS)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../stress stress)
  add_test(NAME stress
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh ${CMAKE_BINARY_DIR} 300 300 10 1000 ${CMAKE_CURRENT_BINARY_DIR}/stress/stress
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/stress
  )
  set_property(TEST stress PROPERTY TIMEOUT_AFTER_MATCH "0.1" "Test timed out")
  set_property(TEST stress PROPERTY TIMEOUT ${FALLBACK_TIMEOUT})
endif()


add_integration_tests("${TEST_LIST}")

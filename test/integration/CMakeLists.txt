cmake_minimum_required(VERSION 3.10)

#to avoid parameters getting mixed between different tests ?
#or does the new "OS cmake fix this.. so we can build many ?"
#cmake_external_project_add(
#  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../net/integration/gateway
#)
## this is where moving things around makes sense..

#TODO add timeout column and make 2 dimensional ?
#TODO create a "single" list of tests ?

#TODO enable pulling everything from conan..

if (NOT DEFINED INCLUDEOS_PREFIX)
  if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
    set(INCLUDEOS_PREFIX /usr/local/includeos)
  else()
    set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
  endif()
endif()


option(STRESS "Enable includeos stress test" OFF)
project(IntegrationTests)

#TODO #INCLUDEOS_PREFIX.-. (NOT NEEDED IF CONAN?) but maybe check it?

#TODO check these and notify if any is missing
find_program(HPING3 hping3)
find_program(NODEJS nodejs) #if not found look for node
#TODO ADD ws4py to deps
enable_testing()

SET(TEST_PARAMS 3)
SET(NAME_PARAM_INDEX    0)
SET(TIMEOUT_PARAM_INDEX 1)
SET(TYPE_PARAM_INDEX    2)

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

  #mod needs lest
  "gsl" "20" "mod"

  #plugin
  "unik" "20" "plugin"

  #posix
  "conf"    "20" "posix"
  #needs lest
  #"file_fd" "20" "posix"
  "main"    "20" "posix"
  #syscall error
  #"pthread" "20" "posix"
  "stat"    "20" "posix"
  "syslog_default" "20" "posix"
  "syslog_plugin" "20" "posix"

  "tcp"   "10"  "posix"
  "udp"   "20"  "posix"

  "utsname" "20" "posix"

  "exceptions" "20" "stl"
  "crt" "20" "stl"
  "stl" "20" "stl"
  "coroutines" "20" "stl"

  "tar_gz" "20" "util"
  "tar" "20" "util"

  #NET tests
  "bufstore"  "30"    "net"
  "configure" "30"    "net"
  "dns"       "20"    "net"
  "gateway"   "50"    "net"
  "http"      "20"    "net"
  "icmp"      "50"    "net"
  "icmp6"     "50"    "net"
  "microLB"   "50"    "net"
  "nat"       "30"    "net"
  #disabled for now..
#  "router"    "30"    "net"
#  "router6"   "30"     "net"
  "slaac"     "30"    "net"
  "tcp"       "120"    "net"
  "udp"       "30"    "net"
  "vlan"      "20"    "net"
  "websocket" "20"    "net"

  #TODO add a cmake variable to exclude these ?
  "dhclient" "20" "net"
  "dhcpd" "20" "net"
  #dhcpd_dhclient_linux

  "block"     "40" "kernel"
  ##TODO check if context test is old and should be removed!!
  #"context"   "20" "kernel"
  "exception" "20" "kernel"
  "fiber"     "20" "kernel"
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

function(add_integration_tests TESTS)
  list(LENGTH TESTS LISTLEN)
  math(EXPR LEN_MINUS_1 '${LISTLEN}-1')
  foreach(INDEX RANGE 0 ${LEN_MINUS_1} ${TEST_PARAMS})
    get_list_param("${TESTS}" ${INDEX} ${NAME_PARAM_INDEX} T)
    get_list_param("${TESTS}" ${INDEX} ${TIMEOUT_PARAM_INDEX} TIMEOUT)
    get_list_param("${TESTS}" ${INDEX} ${TYPE_PARAM_INDEX} TYPE)

    message(STATUS "Adding test integration_${TYPE}_${T} with timeout ${TIMEOUT}")

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../${TYPE}/integration/${T}  ${TYPE}/${T})

    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../${TYPE}/integration/${T}/test.py)
      add_test(NAME integration_${TYPE}_${T}
        #TODO make cmake find python version .. and update vmrunner
        COMMAND python2 -u test.py ${CMAKE_CURRENT_BINARY_DIR}/${TYPE}/${T}/${TYPE}_${T}
        WORKING_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/${TYPE}/${T}
      )
      set_tests_properties(integration_${TYPE}_${T} PROPERTIES TIMEOUT ${TIMEOUT})
      set_tests_properties(integration_${TYPE}_${T} PROPERTIES ENVIRONMENT INCLUDEOS_SRC=${CMAKE_CURRENT_SOURCE_DIR}/../../)
    else()
      message(WARNING "No test.py present in ${CMAKE_CURRENT_SOURCE_DIR}/../${TYPE}/integration/${T}")
    endif()
  endforeach()
endfunction()

if (STRESS)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../stress stress)
  add_test(NAME stress
    COMMAND python2 test.py 300 10 1000 ${CMAKE_CURRENT_BINARY_DIR}/stress/stress
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../stress
  )
  set_tests_properties(stress PROPERTIES TIMEOUT 300)
endif()


add_integration_tests("${TEST_LIST}")

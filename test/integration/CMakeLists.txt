

#to avoid parameters getting mixed between different tests ?
#or does the new "OS cmake fix this.. so we can build many ?"
#cmake_external_project_add(
#  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../net/integration/gateway
#)
## this is where moving things around makes sense..

#TODO add timeout column and make 2 dimensional ?
#TODO create a "single" list of tests ?


set(FS_TESTS
  fat16   20
  fat32   20
  ide     20
  ide_write 20
  memdisk 20
  vfs 20
  virtio_block 30
)
set(HW_TESTS
  serial 20
  vga 20
  virtio_queue 20
)

cmake_minimum_required(VERSION 3.12)

project(IntegrationTests)

#TODO #INCLUDEOS_PREFIX.-. (NOT NEEDED IF CONAN?) but maybe check it?

#TODO check these and notify if any is missing
find_program(HPING3 hping3)
find_program(NODEJS nodejs) #if not found look for node

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
  "bufstore"  "20"    "net"
  "configure" "40"    "net"
  "gateway"   "80"    "net"
  "dns"       "100"   "net"
  "http"      "20"    "net"
  "icmp"      "20"    "net"
  "icmp6"     "20"    "net"
  "nat"       "20"    "net"
  "router"    "20"    "net"
  "tcp"       "20"    "net"
  "udp"       "20"    "net"
  "vlan"      "20"    "net"
  "websocket" "20"    "net"
  #microLB #gonzo how to fix ubsan.. ?? where is the commit..
  #TODO add a cmake variable to exclude these ?
  #dhclient
  #dhcpd
  #dhcpd_dhclient_linux

  "block"     "20" "kernel"
  "context"   "20" "kernel"
  "exception" "20" "kernel"
  "fiber"     "40" "kernel"
  #grub litt magic igjen for å få tak i riktig iso image..
  "kprint"    "40" "kernel"
  #LiveUpdate #gonzo ?
  "memmap"    "80" "kernel"
  #modules GSL path bug with mod2.. fixit!!
  "paging"    "30" "kernel"
  "plugin_init" "25" "kernel"
  "rng"       "32" "kernel"
  "smp"       "24" "kernel"
  "term"      "85" "kernel"
  #timers
  "tls"       "33" "kernel"
)

function(get_index_list LIST WIDTH INDEXLIST)
  list(LENGTH LIST LISTLEN)
  set(MERGELIST)
  math(EXPR LISTITER '${LISTLEN}-1')
  foreach(ELEM RANGE 0 ${LISTITER} ${WIDTH})
    list(GET LIST ${ELEM} INDEX)
    list(APPEND MERGELIST ${INDEX})
  endforeach()
  set(${INDEXLIST} ${MERGELIST} PARENT_SCOPE)
endfunction()

function(get_list_param LIST ITEM INDEX PARAM)
  list(FIND LIST ${ITEM} item_index)
  if (item_idex STREQUAL "-1")
    return()
  endif()
  math(EXPR param_index '${item_index}+${INDEX}')
  list(GET LIST ${param_index} VALUE)
  set(${PARAM} ${VALUE} PARENT_SCOPE)
endfunction()



function(add_integration_tests TESTS)
  get_index_list("${TESTS}" ${TEST_PARAMS} TESTLIST)

  foreach(T ${TESTLIST})
    get_list_param("${TESTS}" ${T} ${TIMEOUT_PARAM_INDEX} TIMEOUT)
    get_list_param("${TESTS}" ${T} ${TYPE_PARAM_INDEX} TYPE)
    message(STATUS "Adding test integration_${TYPE}_${T} with timeout ${TIMEOUT}")
    add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/../${TYPE}/integration/${T}  ${TYPE}/${T})
    add_test(NAME integration_${TYPE}_${T}
      COMMAND python2 test.py ${CMAKE_CURRENT_BINARY_DIR}/${TYPE}/${T}/${T}
      WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/../${TYPE}/integration/${T}
    )
    set_tests_properties(integration_${TYPE}_${T} PROPERTIES TIMEOUT ${TIMEOUT})
  endforeach()
endfunction()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../stress stress)
add_test(NAME stress
  COMMAND python2 test.py 300 10 1000 ${CMAKE_CURRENT_BINARY_DIR}/stress/stress 
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../stress
)
set_tests_properties(stress PROPERTIES TIMEOUT 200)


add_integration_tests("${TEST_LIST}")

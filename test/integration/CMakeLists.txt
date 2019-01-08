cmake_minimum_required(VERSION 3.12)

project(IntegrationTests)

##INCLUDEOS_PREFIX.-. (NOT NEEDED IF CONAN?)

#check these and notify if any is missing
find_program(HPING3 hping3)
find_program(NODEJS nodejs) #if not found look for node

enable_testing()

#to avoid parameters getting mixed between different tests ?
#or does the new "OS cmake fix this.. so we can build many ?"
#cmake_external_project_add(
#  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../net/integration/gateway
#)
## this is where moving things around makes sense..

#TODO add timeout column and make 2 dimensional ?
set(NET_TESTS
  bufstore
  configure
  gateway
  dns
  http
  icmp
  icmp6
  nat
  router
  tcp
  udp
  vlan
  websocket
  #microLB #gonzo how to fix ubsan.. ?? where is the commit..
  #dhclient
  #dhcpd
  #dhcpd_dhclient_linux
)
set(KERNEL_TESTS
  block
  context
  exception
  fiber
  #grub litt magic igjen for å få tak i riktig iso image..
  kprint
  #LiveUpdate #gonzo ?
  memmap
  #modules GSL path bug with mod2.. fixit!!
  paging
  plugin_init
  rng
  smp
  term
  #timers
  tls
)

set(FS_TESTS
  fat16
  fat32
  ide
  ide_write
  memdisk
  vfs
  virtio_block
)
set(HW_TESTS
  serial
  vga
  virtio_queue
)

function(add_integration_tests TYPE TESTS)
  foreach(T ${TESTS})
    add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/../${TYPE}/integration/${T}  ${TYPE}/${T})
    add_test(NAME integration_${TYPE}_${T}
      COMMAND python2 test.py ${CMAKE_CURRENT_BINARY_DIR}/${TYPE}/${T}/${T}
      WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/../${TYPE}/integration/${T}
    )
  endforeach()
endfunction()

add_integration_tests(kernel "${KERNEL_TESTS}")
add_integration_tests(net "${NET_TESTS}")
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../net/integration/dns net/dns)

#add_test(gateway python2 ${CMAKE_CURRENT_SOURCE_DIR}/../net/integration/gateway/test.py ${CMAKE_CURRENT_BINARY_DIR}/net/gateway/gateway)
#add_test(dns python2 ${CMAKE_CURRENT_SOURCE_DIR}/../net/integration/dns/test.py ${CMAKE_CURRENT_BINARY_DIR}/net/dns/dns)

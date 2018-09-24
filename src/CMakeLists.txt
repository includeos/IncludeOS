#
# CMake script for the OS library
#
add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
if (smp)
add_definitions(-DINCLUDEOS_SMP_ENABLE)
endif()

include_directories(${LIBCXX_INCLUDE_DIR})
include_directories(${MUSL_INCLUDE_DIR})
include_directories(${SOLO5_INCLUDE_DIR})
include_directories(${INCLUDEOS_ROOT}/src/include)
include_directories(${INCLUDEOS_ROOT}/api)
include_directories(${INCLUDEOS_ROOT}/mod/)
include_directories(${INCLUDEOS_ROOT}/mod/GSL/)
include_directories(${INCLUDEOS_ROOT}/mod/rapidjson/include)
include_directories(${INCLUDEOS_ROOT}/mod/uzlib/src) # tinf.h for tar
include_directories(${INCLUDEOS_ROOT}/lib/LiveUpdate)
include_directories(${BOTAN_DIR})
include_directories(${OPENSSL_DIR}/include)
if(${ARCH} STREQUAL "x86_64")
  set(OPENSSL_MODULES "net/openssl/init.cpp" "net/openssl/client.cpp"
                      "net/openssl/server.cpp"
                      "net/https/openssl_server.cpp" "net/http/client.cpp")
  set(OPENSSL_LIBS    openssl_ssl openssl_crypto)
endif()
set(BOTAN_MODULES "net/https/botan_server.cpp")

set(OS_OBJECTS
  version.cpp
  kernel/multiboot.cpp
  kernel/syscalls.cpp kernel/os.cpp kernel/cpuid.cpp kernel/block.cpp
  kernel/events.cpp kernel/memmap.cpp kernel/pci_manager.cpp
  kernel/heap.cpp kernel/service_stub.cpp kernel/elf.cpp
  kernel/vga.cpp kernel/context.cpp kernel/context_asm.asm
  kernel/fiber.cpp kernel/tls.cpp kernel/profile.cpp kernel/scoped_profiler.cpp
  kernel/terminal.cpp kernel/timers.cpp kernel/rtc.cpp kernel/rng.cpp
  kernel/system_log.cpp kernel/rdrand.cpp kernel/solo5_manager.cpp
  util/memstream.c util/async.cpp util/statman.cpp "util/statman_liu.cpp"
  util/logger.cpp util/sha1.cpp
  util/syslog_facility.cpp util/syslogd.cpp util/uri.cpp util/percent_encoding.cpp
  util/tar.cpp util/path_to_regex.cpp util/config.cpp util/autoconf.cpp util/crc32.cpp
  crt/c_abi.c crt/ctype_b_loc.c crt/ctype_tolower_loc.c crt/string.c
  crt/quick_exit.cpp crt/cxx_abi.cpp
  hw/pci_device.cpp hw/nic.cpp hw/ps2.cpp hw/serial.cpp hw/vga_gfx.cpp
  hw/msi.cpp hw/pci_msi.cpp virtio/virtio.cpp virtio/virtio_queue.cpp
  net/ethernet/ethernet.cpp net/ethernet/ethernet_8021q.cpp
  net/checksum.cpp net/ip4/arp.cpp net/ip4/ip4.cpp net/ip4/reassembly.cpp
  net/tcp/tcp.cpp net/tcp/connection.cpp net/tcp/connection_states.cpp
  net/tcp/write_queue.cpp net/tcp/rttm.cpp net/tcp/listener.cpp
  net/tcp/read_buffer.cpp net/tcp/read_request.cpp net/tcp/stream.cpp
  net/tcp/tcp_conntrack.cpp
  net/ip4/icmp4.cpp net/ip4/udp.cpp net/ip4/udp_socket.cpp
  net/ip6/ip6.cpp net/ip6/icmp6.cpp net/ip6/ndp.cpp
  net/dns/dns.cpp net/dns/client.cpp net/dhcp/dh4client.cpp net/dhcp/dhcpd.cpp
  net/buffer_store.cpp net/inet.cpp
  net/super_stack.cpp net/configure.cpp net/conntrack.cpp net/vlan_manager.cpp
  net/http/header.cpp net/http/header_fields.cpp net/http/message.cpp net/http/request.cpp
  net/http/response.cpp net/http/status_codes.cpp net/http/time.cpp net/http/version.cpp
  net/http/mime_types.cpp net/http/cookie.cpp
  net/http/client_connection.cpp net/http/basic_client.cpp
  net/http/server_connection.cpp net/http/server.cpp net/http/response_writer.cpp
  net/ws/websocket.cpp ${OPENSSL_MODULES} ${BOTAN_MODULES}
  net/nat/nat.cpp net/nat/napt.cpp
  fs/disk.cpp fs/filesystem.cpp fs/dirent.cpp fs/mbr.cpp fs/path.cpp
  fs/fat.cpp fs/fat_async.cpp fs/fat_sync.cpp fs/memdisk.cpp
  # POSIX
  posix/fd.cpp posix/file_fd.cpp posix/tcp_fd.cpp posix/udp_fd.cpp posix/unix_fd.cpp

)

add_library(os STATIC ${OS_OBJECTS})
add_dependencies(os PrecompiledLibraries botan ${OPENSSL_LIBS})

# disable sanitizers on c_abi and cxx_abi, etc.
set_source_files_properties(crt/c_abi.c PROPERTIES COMPILE_FLAGS "-fno-sanitize=all")
set_source_files_properties(crt/cxx_abi.cpp PROPERTIES COMPILE_FLAGS "-fno-sanitize=all")
set_source_files_properties(version.cpp PROPERTIES COMPILE_DEFINITIONS "OS_VERSION=\"${OS_VERSION}\"")

add_subdirectory(arch/${ARCH})
add_subdirectory(platform/x86_pc)
add_subdirectory(platform/x86_nano)
if(WITH_SOLO5)
add_subdirectory(platform/x86_solo5)
endif(WITH_SOLO5)
add_subdirectory(drivers)
add_subdirectory(plugins)

# Add musl
add_subdirectory(musl)

#
# Installation
#
set(CMAKE_INSTALL_MESSAGE LAZY) # to avoid spam
install(TARGETS os DESTINATION includeos/${ARCH}/lib)

install(DIRECTORY ${INCLUDEOS_ROOT}/src/memdisk/ DESTINATION includeos/memdisk
        FILES_MATCHING PATTERN "*.*")

install(FILES service_name.cpp DESTINATION includeos/src)

# object format needs to be set BEFORE enabling ASM
# see: https://cmake.org/Bug/bug_relationship_graph.php?bug_id=13166
if ("${ARCH}" STREQUAL "i686")
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf")
  set(OBJCOPY_TARGET "elf32-i386")
else()
  set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf64")
  set(OBJCOPY_TARGET "elf64-x86-64")
endif()

enable_language(ASM_NASM)


add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")
if (single_threaded)
add_definitions(-DINCLUDEOS_SINGLE_THREADED)
endif()

include_directories(${INCLUDEOS_ROOT}/api/posix)
include_directories(${LIBCXX_INCLUDE_DIR})
include_directories(${NEWLIB_INCLUDE_DIR})
include_directories(${SOLO5_INCLUDE_DIR})
include_directories(${INCLUDEOS_ROOT}/src/include)
include_directories(${INCLUDEOS_ROOT}/api)
include_directories(${INCLUDEOS_ROOT}/mod/)
include_directories(${INCLUDEOS_ROOT}/mod/GSL/)
include_directories(${INCLUDEOS_ROOT}/mod/rapidjson/include)
include_directories(${INCLUDEOS_ROOT}/mod/uzlib/src) # tinf.h for tar
include_directories(${BOTAN_DIR})


# TODO: i wanted to use a glob, but then found out that not everything is included
set(OS_OBJECTS
  kernel/kernel_start.cpp kernel/syscalls.cpp
  kernel/irq_manager.cpp kernel/multiboot.cpp
  kernel/os.cpp kernel/cpuid.cpp kernel/memmap.cpp kernel/pci_manager.cpp
  kernel/heap.cpp kernel/service_stub.cpp kernel/main_call.cpp
  kernel/elf.cpp kernel/terminal.cpp kernel/terminal_disk.cpp
  kernel/vga.cpp kernel/context.cpp kernel/context_asm.asm
  kernel/fiber.cpp kernel/tls.cpp
  kernel/profile.cpp
  kernel/timers.cpp kernel/rtc.cpp kernel/rng.cpp kernel/rdrand.cpp
  kernel/solo5_manager.cpp
  util/memstream.c util/async.cpp util/statman.cpp util/logger.cpp util/sha1.cpp
  util/syslog_facility.cpp util/syslogd.cpp util/uri.cpp util/percent_encoding.cpp
  util/tar.cpp util/path_to_regex.cpp
  crt/c_abi.c crt/string.c crt/quick_exit.cpp crt/cxx_abi.cpp
  hw/ide.cpp hw/pci_device.cpp hw/ps2.cpp hw/serial.cpp
  hw/cmos.cpp hw/block_device.cpp hw/msi.cpp hw/pci_msi.cpp
  virtio/virtio.cpp virtio/virtio_queue.cpp
  net/ethernet/ethernet.cpp net/inet_common.cpp net/ip4/arp.cpp net/ip4/ip4.cpp
  net/tcp/tcp.cpp net/tcp/connection.cpp net/tcp/connection_states.cpp
  net/tcp/write_queue.cpp net/tcp/rttm.cpp net/tcp/listener.cpp net/tcp/read_buffer.cpp
  net/ip4/icmp4.cpp net/ip4/udp.cpp net/ip4/udp_socket.cpp
  net/dns/dns.cpp net/dns/client.cpp net/dhcp/dh4client.cpp net/dhcp/dhcpd.cpp
  net/buffer_store.cpp net/inet4.cpp
  net/super_stack.cpp net/autoconf.cpp
  net/http/header.cpp net/http/header_fields.cpp net/http/message.cpp net/http/request.cpp
  net/http/response.cpp net/http/status_codes.cpp net/http/time.cpp net/http/version.cpp
  net/http/mime_types.cpp net/http/cookie.cpp net/http/secure_server.cpp
  net/http/client_connection.cpp net/http/client.cpp
  net/http/server_connection.cpp net/http/server.cpp net/http/response_writer.cpp
  net/ws/websocket.cpp net/nat/napt.cpp
  fs/disk.cpp fs/filesystem.cpp fs/dirent.cpp fs/mbr.cpp fs/path.cpp
  fs/fat.cpp fs/fat_async.cpp fs/fat_sync.cpp fs/memdisk.cpp
  posix/fd.cpp posix/tcp_fd.cpp posix/udp_fd.cpp posix/unistd.cpp posix/fcntl.cpp posix/syslog.cpp
  posix/sys/socket.cpp posix/sys/select.cpp posix/sys/utsname.cpp posix/sys/mman.cpp posix/arpa/inet.cpp
  posix/ucontext.cpp posix/ucontext_asm.asm
  posix/sys/stat.cpp posix/ftw.cpp posix/file_fd.cpp posix/dlfcn.cpp
  posix/pwd.cpp posix/pthread.cpp)

add_library(os STATIC ${OS_OBJECTS})
add_dependencies(os PrecompiledLibraries botan)

add_subdirectory(arch/${ARCH})
add_subdirectory(platform/x86_pc)
add_subdirectory(platform/x86_nano)
if(WITH_SOLO5)
add_subdirectory(platform/x86_solo5)
endif(WITH_SOLO5)
add_subdirectory(drivers)
add_subdirectory(plugins)

# add_dependencies(arch/${ARCH} PrecompiledLibraries)
# add_dependencies(drivers PrecompiledLibraries)
# add_dependencies(plugins PrecompiledLibraries)


install(TARGETS os DESTINATION includeos/${ARCH}/lib)

install(DIRECTORY ${INCLUDEOS_ROOT}/src/memdisk/ DESTINATION includeos/memdisk
        FILES_MATCHING PATTERN "*.*")

install(FILES service_name.cpp DESTINATION includeos/src)

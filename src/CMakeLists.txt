set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf")
enable_language(ASM_NASM)

include_directories(${INCLUDEOS_ROOT}/api/sys)
include_directories(${INCLUDEOS_ROOT}/api/posix)
include_directories(${INCLUDEOS_INSTALL_DIR}/libcxx/include)
include_directories(${INCLUDEOS_INSTALL_DIR}/newlib/include)
include_directories(${INCLUDEOS_ROOT}/src/include)
include_directories(${INCLUDEOS_ROOT}/api)
include_directories(${INCLUDEOS_ROOT}/mod/GSL/)

add_custom_command(
	OUTPUT apic_boot.o
	COMMAND nasm -f bin -o apic_boot.bin ${CMAKE_CURRENT_SOURCE_DIR}/hw/apic_boot.asm
	COMMAND objcopy -I binary -O elf32-i386 -B i386 apic_boot.bin apic_boot.o
	DEPENDS hw/apic_boot.asm
)

# TODO: i wanted to use a glob, but then found out that not everything is included
set(OS_OBJECTS
  kernel/start.asm kernel/kernel_start.cpp kernel/syscalls.cpp
  kernel/interrupts.asm kernel/os.cpp kernel/os_stdout.cpp kernel/cpuid.cpp 
  kernel/memmap.cpp kernel/irq_manager.cpp kernel/pci_manager.cpp 
  kernel/elf.cpp kernel/terminal.cpp kernel/terminal_disk.cpp 
  kernel/vga.cpp kernel/context.cpp kernel/context_asm.asm
  kernel/profile.cpp kernel/profile_intr.asm
  kernel/timers.cpp kernel/rtc.cpp
  util/memstream.c util/async.cpp util/statman.cpp util/logger.cpp
  crt/c_abi.c crt/string.c crt/quick_exit.cpp crt/cxx_abi.cpp
  hw/ide.cpp hw/pit.cpp hw/pic.cpp hw/pci_device.cpp hw/cpu_freq_sampling.cpp
  hw/ps2.cpp hw/serial.cpp hw/cmos.cpp hw/drive.cpp
  hw/acpi.cpp hw/apic.cpp hw/apic_timer.cpp hw/ioapic.cpp hw/apic_asm.asm
  hw/apic_revenant.cpp hw/msi.cpp hw/pci_msi.cpp
  virtio/virtio.cpp virtio/virtio_queue.cpp
  net/ethernet/ethernet.cpp net/inet_common.cpp net/ip4/arp.cpp net/ip4/ip4.cpp
  net/tcp/tcp.cpp net/tcp/connection.cpp net/tcp/connection_states.cpp
  net/tcp/rttm.cpp net/tcp/listener.cpp
  net/ip4/icmpv4.cpp net/ip4/udp.cpp net/ip4/udp_socket.cpp
  net/dns/dns.cpp net/dns/client.cpp net/dhcp/dh4client.cpp
  net/buffer_store.cpp net/inet4.cpp
  fs/disk.cpp fs/filesystem.cpp fs/mbr.cpp fs/path.cpp
  fs/ext4.cpp fs/fat.cpp fs/fat_async.cpp fs/fat_sync.cpp fs/memdisk.cpp
  posix/tcp_fd.cpp posix/udp_fd.cpp posix/unistd.cpp posix/fcntl.cpp
  posix/sys/socket.cpp posix/sys/mman.cpp posix/arpa/inet.cpp
  )

add_library(os STATIC ${OS_OBJECTS} apic_boot.o)

file(GLOB CXX_ABI crt/cxxabi/*.cpp)

add_library(libc++abi.a STATIC ${CXX_ABI})

add_library(libcrt OBJECT crt/crti.s crt/crtn.s)

add_subdirectory(boot)
add_subdirectory(drivers)

# TODO: what is this testdisk?
#
# build memdisk
#add_custom_command(
	#OUTPUT memdisk
	#COMMAND nasm -f elf -o memdisk ${CMAKE_CURRENT_SOURCE_DIR}/memdisk.asm
	#DEPENDS memdisk.asm
	#)
#
#add_custom_target(create_memdisk ALL DEPENDS memdisk)

#################################################
#              IncludeOS makefile               #
#################################################

ifndef INCLUDEOS_INSTALL
	INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
	#$(error IncludeOS installation folder not set!)
endif
# shorter name
INSTALL = $(INCLUDEOS_INSTALL)

STACK_PROTECTOR_VALUE=$(shell date +%s)
# stackrealign is needed to guarantee 16-byte stack alignment for SSE
# the compiler seems to be really dumb in this regard, creating a misaligned stack left and right
CAPABS_COMMON = -mstackrealign -msse3 -fstack-protector-strong -D_STACK_GUARD_VALUE_=$(STACK_PROTECTOR_VALUE)

# Various global defines
# * NO_DEBUG Disables  output from the debug macro
# * OS_TERMINATE_ON_CONTRACT_VIOLATION provides classic assert-like output from Expects / Ensures
# * _GNU_SOURCE enables POSIX-extensions in newlib, such as strnlen. ("everything newlib has", ref. cdefs.h)
CAPABS = $(CAPABS_COMMON) -DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE
WARNS   = -Wall -Wextra #-pedantic
DEBUG_OPTS = -ggdb3


# External Libraries
###################################################
LIBC_OBJ = $(INSTALL)/newlib/libc.a
LIBG_OBJ = $(INSTALL)/newlib/libg.a
LIBM_OBJ = $(INSTALL)/newlib/libm.a

LIBGCC = $(INSTALL)/libgcc/libgcc.a
LIBCXX = $(INSTALL)/libcxx/libc++.a libc++abi.a

INC_NEWLIB=$(INSTALL)/newlib/include

# Compiler/Linker
###################################################

CC  = $(shell command -v clang-3.8 || command -v clang-3.6)
CPP = $(shell command -v clang++-3.8 || command -v clang++-3.6 || command -v clang++)
# Set defaults if not defined
ifndef LD_INC
	LD_INC = ld
endif
ifndef AR_INC
	AR_INC = ar
endif
ifndef OBJCOPY_INC
	OBJCOPY_INC=objcopy
endif

# Compiler options
CCOPTS  = -target i686-elf
CPPOPTS = -target i686-elf

INCLUDES  = -I../api/sys -I$(INSTALL)/libcxx/include -I$(INC_NEWLIB) -Iinclude -I../api -I../mod/GSL
CINCLUDES = -I../api/sys -I$(INC_NEWLIB) -Iinclude -I../api

CCOPTS  += $(CAPABS) $(WARNS) -c -m32 -march=i686 $(CINCLUDES)
CPPOPTS += $(CAPABS) $(WARNS) -c -m32 -std=c++14 $(INCLUDES) -D_LIBCPP_HAS_NO_THREADS=1 -DOS_VERSION="\"$(shell git describe --dirty)\""
LDOPTS = -nostdlib -melf_i386 -N --eh-frame-hdr --script=linker.ld

# Objects
###################################################
CXXABI     = $(shell find ./crt/cxxabi -type f -name "*.cpp")
CXXABI_OBJ = $(CXXABI:.cpp=.o)

OS_OBJECTS = kernel/start.o kernel/kernel_start.o kernel/syscalls.o \
		kernel/interrupts.o kernel/os.o kernel/cpuid.o kernel/memmap.o\
		kernel/irq_manager.o kernel/pci_manager.o \
		kernel/elf.o kernel/terminal.o kernel/terminal_disk.o \
		kernel/vga.o kernel/debug_new.o \
		kernel/profile.o kernel/profile_intr.o \
		kernel/timers.o kernel/rtc.o util/memstream.o util/async.o util/statman.o \
		crt/c_abi.o crt/string.o crt/quick_exit.o crt/cxx_abi.o crt/mman.o \
		hw/ide.o hw/pit.o hw/pic.o hw/pci_device.o hw/cpu_freq_sampling.o \
		hw/ps2.o hw/serial.o hw/cmos.o hw/drive.o \
		hw/acpi.o hw/apic.o hw/apic_timer.o hw/ioapic.o hw/apic_asm.o \
		hw/apic_revenant.o hw/msi.o hw/pci_msi.o \
		virtio/virtio.o virtio/virtio_queue.o \
		net/ethernet.o net/inet_common.o net/ip4/arp.o net/ip4/ip4.o \
		net/tcp/tcp.o net/tcp/connection.o net/tcp/connection_states.o \
		net/tcp/rttm.o net/tcp/listener.o \
		net/ip4/icmpv4.o net/ip4/udp.o net/ip4/udp_socket.o \
		net/dns/dns.o net/dns/client.o net/dhcp/dh4client.o \
		net/ip6/ip6.o net/ip6/icmp6.o net/ip6/udp6.o net/ip6/ndp.o \
		net/buffer_store.o net/inet4.o \
		fs/disk.o fs/filesystem.o fs/mbr.o fs/path.o \
		fs/ext4.o fs/fat.o fs/fat_async.o fs/fat_sync.o fs/memdisk.o
		# virtio/console.o

DRIVERS = drivers/virtioblk.o drivers/virtionet.o
DRIVERS_DEPS = $(DRIVERS:.o=.d)

PLATFORMS = platforms/unik.o
PLATFORMS_DEPS = $(PLATFORMS:.o=.d)

CRTI_OBJ = crt/crti.o
CRTN_OBJ = crt/crtn.o

# Header dependencies (rebuild if header changed)
###################################################
OS_DEPS = $(OS_OBJECTS:.o=.d)

.PHONY: memdisk all debug stripped minimal clean

# Complete OS build
###################################################
# A complete build includes:
# - a bootloader
# - an OS library for the service to link against

all: CAPABS += -O2
all: includeos
	@echo "\n>>> Built OS-library. Install to '"$(INSTALL)"' using 'make install'"

stripped: CAPABS += -Os
stripped: LDOPTS += -s
stripped: includeos

# Build like "all" but with debugging output (i.e. the 'debug'-macro) enabled
debug-info: CAPABS += -UNO_DEBUG
debug-info: CAPABS += -DGSL_THROW_ON_CONTRACT_VIOLATION
debug-info: includeos test

# Build with debugging symbols (OBS: Dramatically increases binary size)
debug: CAPABS += -O0
debug: CCOPTS  += $(DEBUG_OPTS)
debug: CPPOPTS += $(DEBUG_OPTS)
debug: OBJ_LIST += $(LIBG_OBJ)
debug: includeos test

# Build with debugging symbols + debugging ouput, i.e. "debug" + "debug-info"
debug-all: CAPABS += -O0 -UNO_DEBUG
debug-all: CCOPTS  += $(DEBUG_OPTS)
debug-all: CPPOPTS += $(DEBUG_OPTS)
debug-all: OBJ_LIST += $(LIBG_OBJ)
debug-all: includeos test

silent: CPPOPTS += -DNO_INFO=1
silent: includeos

minimal: CPPOPTS += -ffunction-sections -fdata-sections
minimal: LDOPTS += --gc-sections
minimal: stripped

includeos: bootloader multiboot libc++abi.a os.a drivers platforms

# Test service
###################################################
LIBS_OBJ = os.a $(LIBCXX) os.a $(LIBC_OBJ) $(LIBM_OBJ) $(LIBGCC)


CRTBEGIN_OBJ = $(CRTI_OBJ) $(INSTALL)/crt/crtbegin.o
CRTEND_OBJ   = $(INSTALL)/crt/crtend.o $(CRTN_OBJ)

TEST_OBJ = debug/test_service.o util/service_name.o

test_service: CPPOPTS += -DSERVICE_NAME="\"Test Service\""
test_service: $(TEST_OBJ) $(CRTI_OBJ) $(CRTN_OBJ)
	@echo "\n>> Linking test service"
	@echo "\n>> LDOPTS = $(LDOPTS)"
	touch smalldisk
	$(LD_INC) -v $(LDOPTS) $(CRTBEGIN_OBJ) $(TEST_OBJ) $(LIBS_OBJ) $(CRTEND_OBJ) -o debug/$@

test: test_service

# OS libraries
###################################################
# OS
libc++abi.a: $(CXXABI_OBJ)
	$(AR_INC) rcs $@ $(CXXABI_OBJ)

os.a: $(OS_OBJECTS) apic_boot.o
	@echo "\n>> Building the OS library"
	$(AR_INC) rcs $@ $(OS_OBJECTS) apic_boot.o

drivers: $(DRIVERS)

platforms: $(PLATFORMS)

install: drivers platforms bootloader multiboot $(CRTI_OBJ) $(CRTN_OBJ)
	@echo "\n>> Installing IncludeOS"
# API
	cp -r ../api $(INSTALL)/
# Drivers
	mkdir -p $(INSTALL)/drivers
	cp -r drivers/*.o $(INSTALL)/drivers
# Platforms
	mkdir -p $(INSTALL)/platforms
	cp -r platforms/*.o $(INSTALL)/platforms
# Boot related
	cp bootloader $(INSTALL)
	cp boot/multiboot.o $(INSTALL)
# Core libraries
	cp os.a $(INSTALL)
	cp libc++abi.a $(INSTALL)/libcxx
	cp crt/crti.o $(INSTALL)/crt
	cp crt/crtn.o $(INSTALL)/crt
# Linker script
	cp linker.ld $(INSTALL)/
# Memdisk related
	mkdir -p $(INSTALL)/memdisk
	cp memdisk/* $(INSTALL)/memdisk/
# Seed related
	cp seed/service_name.cpp $(INSTALL)
	cp seed/Makefile $(INSTALL)/Makeseed
	cp seed/Makelib $(INSTALL)/Makelib
# External dependencies related
	mkdir -p $(INSTALL)/mod/
	cp -r ../mod/GSL $(INSTALL)/mod
	@echo "\nDone!"

# Makefile recipes
###################################################
%.o: %.c
	$(CC) -MMD $(CCOPTS) -o $@ $<

%.o: %.cpp
	@echo "\n"
	$(CPP) -MMD $(CPPOPTS) -o $@ $<

%.o: %.s
	@echo "\n"
	$(CPP) $(CPPOPTS) -x assembler-with-cpp $< -o $@

%.o: %.asm
	nasm -f elf $< -o $@

# Bootloaders
###################################################
bootloader: boot/bootloader.asm boot/disk_read_lba.asm
	@echo "\n>> Assembling bootloader"
	nasm -f bin -o bootloader $<

apic_boot.o: hw/apic_boot.asm
	nasm -f bin -o apic_boot.bin $<
	$(OBJCOPY_INC) -I binary -O elf32-i386 -B i386 apic_boot.bin $@

multiboot: boot/multiboot.o

# Disk image as a section
###################################################
memdisk: memdisk.asm
	@echo "\n>> Assembling memdisk"
	nasm -f elf -o memdisk.o $<

# Etags
###################################################
etags:
	@echo "\n>> Updating emacs tags "
	- find ../ -name "*.[hc]pp" | grep -v ".*\#.*" | etags -


# Cleanup
###################################################
clean_tests:
	$(RM) debug/*.o debug/*.d debug/*.img debug/test_service debug/test_ipv6 debug/test_tcp

clean: clean_tests
	$(RM) $(OS_OBJECTS) $(CXXABI_OBJ) $(OS_DEPS) $(DRIVERS) $(DRIVERS_DEPS) $(PLATFORMS) $(PLATFORMS_DEPS)
	$(RM) os.a libc++abi.a
	$(RM) bootloader apic_boot apic_boot.o memdisk.o boot/multiboot.o

-include $(OS_DEPS)

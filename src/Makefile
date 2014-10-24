# The name of your service
SERVICE="IncludeOS_tests"

# Compiler/Linker
###################################################
OS = /usr/local/IncludeOS
OSLIB = $(OS)/i686-elf

DEBUG_OPTS = -ggdb3

CC  = $(OS)/bin/i686-elf-gcc 
CPP = $(OS)/bin/i686-elf-g++ 
LD  = $(OS)/bin/i686-elf-ld 

GCC_VER = $(shell $(CC) -dumpversion)
LIBGCC = $(OSDIR)/lib/gcc/i686-elf/$(GCC_VER)/libgcc.a

INCLUDES = -I$(OSLIB)/include -I../abi/ -Iinclude -I../stdlib
LIBS     = $(OSLIB)/lib $(LIBGCC)

CAPABS  = -O2 -msse3
WARNS   = -Wall -Wextra
CCOPTS	= $(CAPABS) $(WARNS) -c -m32 -fno-stack-protector -fno-builtin -march=i386 $(INCLUDES) -lgcc
CPPOPTS = $(CAPABS) $(WARNS) -c -std=c++11 -ffreestanding -nostdlib -fno-builtin -fno-exceptions -fno-rtti $(INCLUDES)

LDOPTS = -nostdlib -melf_i386 -N  --script=linker.ld 


# Objects
###################################################
TEST_OBJ= tests/tests.o

OS_OBJECTS=kernel_start.o syscalls.o interrupts.o class_os.o \
	crt/c_abi.o crt/cxx_abi.o crt/string.o crt/iostream.o \
	crt/EASTL/allocator.o crt/EASTL/assert.o crt/EASTL/fixed_pool.o \
	crt/EASTL/hashtable.o crt/EASTL/red_black_tree.o crt/EASTL/string.o \
	hw/class_device.o hw/class_pci_device.o hw/pic.o \
	class_irq_handler.o class_pci_manager.o \
	virtio/class_virtio.o virtio/class_virtionet.o virtio/class_virtio_queue.o \
	net/class_ethernet.o net/class_arp.o net/class_ip4.o net/class_ip6.o\
	net/class_udp.o net/class_icmp.o net/inet.o \
	SQLite/sqlite.o SQLite/sqlite3.o SQLite/sqlite_fixes.o \
	$(TEST_OBJ) 
#virtio/virtionet.o virtio/virtio.o
 #hw/pci.o class_pci_manager_sanos.o  hw/class_nic.o

# Libraries
###################################################
LIBC_OBJ = $(OSLIB)/lib/libc.a
LIBG_OBJ = $(OSLIB)/lib/libg.a 
LIBM_OBJ = $(OSLIB)/lib/libm.a 


# Header dependencies (rebuild if header changed)
###################################################
OS_DEPS=$(OS_OBJECTS:.o=.d)


# Complete bulid
###################################################
# A complete build includes:
# - a bootloader
# - an OS library for the service to link against
all: bootloader os.a etags

stripped: LDOPTS += -s #strip all
stripped: bootloader os.a


# The same, but with debugging symbols (OBS: Dramatically increases binary size)
debug: CCOPTS  += $(DEBUG_OPTS)
debug: CPPOPTS += $(DEBUG_OPTS)
debug: LDOPTS  += -M --verbose

debug: OBJ_LIST += $(LIBG_OBJ)

debug: bootloader os  #Don't wanna call 'all', since it strips debug info

# OS
os.a: $(OS_OBJECTS)
	@echo "\n>> Building the OS library"
	ar rcs $@ $(OS_OBJECTS)

install: os.a
	@echo "\n>> Installing..."
	cp os.a $(OS)/lib
	cp bootloader $(OS)/
	mkdir -p $(OS)/crt
	cp crt/crt*.s $(OS)/crt
	cp -r ../abi $(OS)/
	cp -r ../stdlib $(OS)/
	cp linker.ld $(OS)/
	@echo "\nDone!"

# Object files
###################################################
# General C++-files to object files. The OS is compiled here.

%.o: %.cpp 
	@echo "\n>> Compiling " $@
	$(CPP) -MMD $(CPPOPTS) -o $@ $< 

# AS-assembled object files
%.o: %.s
	@echo "\n>> Assembling GNU 'as' files"
	$(CPP) $(CPPOPTS) -x assembler-with-cpp $<

# Bootloader
###################################################
# Nasm-based bootloader
bootloader: bootloader.asm
	@echo "\n>> Assembling bootloader"
	nasm -f bin -o bootloader bootloader.asm

# Optional, for gcc-assembled bootloader
bootloader_s: bootloader.s
	$(CC) $(CCOPTS) -o $@ $<


# Etags
###################################################
etags:
	@echo "\n>> Updating emacs tags "
	- find ../ -name "*.[hc]pp" | grep -v ".*\#.*" | etags -


# Cleanup
###################################################
clean: 
	$(RM) $(OS_OBJECTS) $(OS_DEPS)
	$(RM) $(SERVICE)*
	$(RM) os.a
	$(RM) bootloader	

-include $(OS_DEPS)

  #################################################
 #              IncludeOS makefile               #
#################################################

# Compiler/Linker
###################################################
OS = /usr/local/IncludeOS
OSLIB = $(OS)/i686-elf

DEBUG_OPTS = -ggdb3

CC  = $(OS)/bin/i686-elf-gcc
CPP = $(OS)/bin/i686-elf-g++
LD  = $(OS)/bin/i686-elf-ld

GCC_VER = $(shell $(CC) -dumpversion)
LIBGCC = $(OS)/lib/gcc/i686-elf/$(GCC_VER)/libgcc.a

INCLUDES = -I$(OSLIB)/include -I../abi -Iinclude -I../stdlib
LIBS     = $(OSLIB)/lib $(LIBGCC)

CAPABS  = -Ofast -msse3 -mstackrealign  # Needed for 16-byte stack alignment (SSE)
WARNS   = -Wall -Wextra -pedantic
CCOPTS	= $(CAPABS) $(WARNS) -c -m32 -fno-stack-protector -fno-builtin -march=i686 $(INCLUDES) -lgcc
CPPOPTS = $(CAPABS) $(WARNS) -c -m32 -std=c++11 -ffreestanding -nostdlib -fno-builtin $(INCLUDES)

LDOPTS = -ffreestanding -nostdlib -melf_i386 -N --script=linker.ld 


# Objects
###################################################
CXXABI     = $(shell find ./crt/cxxabi -type f -name "*.cpp")

OS_OBJECTS = kernel_start.o syscalls.o interrupts.o class_os.o \
						 crt/c_abi.o util/memstream.o \
						 class_irq_handler.o class_pci_manager.o \
						 hw/pic.o hw/class_pci_device.o hw/class_dev.o \
						 virtio/class_virtio.o virtio/class_virtionet.o virtio/class_virtio_queue.o \
						 net/class_ethernet.o net/class_arp.o net/class_ip4.o net/class_ip6.o\
						 net/class_udp.o net/class_icmp.o net/class_inet.o net/inet.o\
						 net/class_packet.o net/dns/dns.o

# Libraries
###################################################
LIBC_OBJ = $(OSLIB)/lib/libc.a
LIBG_OBJ = $(OSLIB)/lib/libg.a 
LIBM_OBJ = $(OSLIB)/lib/libm.a 

# Header dependencies (rebuild if header changed)
###################################################
OS_DEPS=$(OS_OBJECTS:.o=.d)


# Complete build
###################################################
# A complete build includes:
# - a bootloader
# - an OS library for the service to link against
all: bootloader libc++abi.a os.a etags
	echo "TEST: OSLIB: '"$(OS)"'"

stripped: LDOPTS += -s #strip all
stripped: bootloader os.a


# The same, but with debugging symbols (OBS: Dramatically increases binary size)
debug: CCOPTS  += $(DEBUG_OPTS)
debug: CPPOPTS += $(DEBUG_OPTS)
debug: LDOPTS  += -M --verbose

debug: OBJ_LIST += $(LIBG_OBJ)

debug: bootloader os  #Don't wanna call 'all', since it strips debug info

# OS

libc++abi.a: $(CXXABI:.cpp=.o)
	ar rcs $@ $(CXXABI:.cpp=.o)

os.a: $(OS_OBJECTS)
	@echo "\n>> Building the OS library"
	ar rcs $@ $(OS_OBJECTS)

install: #os.a bootloader
	@echo "\n>> Installing..."
	cp bootloader $(OS)/
	cp os.a $(OS)/lib
	cp libc++abi.a $(OS)/lib
	cp libc++.a $(OS)/lib
	mkdir -p $(OS)/crt
	cp crt/crt*.s $(OS)/crt
	cp -r ../abi $(OS)/
	cp -r ../stdlib $(OS)/
	mkdir -p $(OS)/mod
	#cp -r ../mod/rapidjson $(OS)/mod
	#cp -r ../mod/protobuf $(OS)/mod
	cp linker.ld $(OS)/
	@echo "\nDone!"

# Object files
###################################################
%.o: %.c
	$(CC) -MMD $(CCOPTS) -o $@ $< 

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
	$(RM) os.a
	$(RM) bootloader	

-include $(OS_DEPS)

#################################################
#              IncludeOS makefile               #
#################################################

INSTALL = ../../IncludeOS_install

# External Libraries
###################################################
LIBC_OBJ = $(INSTALL)/newlib/libc.a
LIBG_OBJ = $(INSTALL)/newlib/libg.a
LIBM_OBJ = $(INSTALL)/newlib/libm.a 

LIBGCC = $(INSTALL)/libgcc/libgcc.a
LIBCXX = $(INSTALL)/libcxx/libc++.a libc++abi.a


INC_NEWLIB=$(INSTALL)/newlib/include

DEBUG_OPTS = -ggdb3

# Compiler/Linker
###################################################

CC  = clang-3.6 -target i686-elf
CPP = clang++-3.6 -target i686-elf
LD  = ld # Yes - we use the default linker

INCLUDES = -I../api/sys -I$(INC_NEWLIB) -Iinclude -I../api -I../stdlib #-I$(INSTALL)/libcxx/include 

CAPABS_COMMON = -msse3 -mstackrealign # Needed for 16-byte stack alignment (SSE)

all: CAPABS  =  $(CAPABS_COMMON) -O2  
debug: CAPABS = $(CAPABS_COMMON) -O0
stripped: CAPABS = $(CAPABS_COMMON) -Oz

WARNS   = -Wall -Wextra #-pedantic
CCOPTS	= $(CAPABS) $(WARNS) -c -m32 -fno-stack-protector -fno-builtin -march=i686 $(INCLUDES)
CPPOPTS = $(CAPABS) $(WARNS) -c -m32 -std=c++14 -fno-stack-protector $(INCLUDES) #-flto -fno-exceptions

LDOPTS =  -nostdlib -melf_i386 -N --eh-frame-hdr --script=linker.ld #-flto 

# Objects
###################################################
CXXABI     = $(shell find ./crt/cxxabi -type f -name "*.cpp")
CXXABI_OBJ = $(CXXABI:.cpp=.o)

OS_OBJECTS = 	kernel/kernel_start.o kernel/syscalls.o \
		kernel/interrupts.o kernel/os.o \
		kernel/irq_manager.o kernel/pci_manager.o kernel/pit.o \
		crt/c_abi.o crt/string.o crt/quick_exit.o crt/cxx_abi.o util/memstream.o \
		hw/pic.o hw/class_pci_device.o hw/class_dev.o hw/cpu_freq_sampling.o \
		virtio/class_virtio.o virtio/class_virtionet.o virtio/class_virtio_queue.o \
		net/class_ethernet.o net/inet.o net/class_inet.o net/class_arp.o net/class_ip4.o \
		net/class_udp.o net/class_tcp.o net/class_tcp_socket.o net/class_icmp.o net/class_inet4.o \
		net/ip6/ip6.o net/ip6/icmp6.o net/ip6/udp6.o net/ip6/ndp.o \
		net/class_packet.o net/dns/dns.o 

# Header dependencies (rebuild if header changed)
###################################################
OS_DEPS = $(OS_OBJECTS:.o=.d)


# Complete OS build
###################################################
# A complete build includes:
# - a bootloader
# - an OS library for the service to link against

all: bootloader libc++abi.a os.a 
	@echo "\n>>> Built OS-library. Install to'"$(OS)"' using 'make install'"

#stripped: LDOPTS += -s #strip all
stripped: bootloader libc++abi.a os.a test # Included test to get a stripped version


# The same, but with debugging symbols (OBS: Dramatically increases binary size)
debug: CCOPTS  += $(DEBUG_OPTS)
debug: CPPOPTS += $(DEBUG_OPTS)
#debug: LDOPTS  += #-M #--verbose
debug: OBJ_LIST += $(LIBG_OBJ)
debug: bootloader libc++abi.a os.a # Can't call all, since it changes capabs
debug: test

# Test service
###################################################
LIBS_OBJ = os.a $(LIBCXX) os.a $(LIBC_OBJ) $(LIBM_OBJ) $(LIBGCC)


CRTI_OBJ     = crt/crti.o
CRTBEGIN_OBJ = $(INSTALL)/crt/crtbegin.o
CRTEND_OBJ   = $(INSTALL)/crt/crtend.o
CRTN_OBJ     = crt/crtn.o

test_service: debug/test_service.o $(CRTI_OBJ) $(CRTN_OBJ)
	@echo "\n>> Linking test service"
	$(LD) -v $(LDOPTS) $(CRTI_OBJ) $(CRTBEGIN_OBJ) debug/test_service.o $(LIBS_OBJ) $(CRTEND_OBJ) $(CRTN_OBJ) -o debug/$@

test_ipv6: debug/test_ipv6.o $(CRTI_OBJ) $(CRTN_OBJ)
	@echo "\n>> Linking test IPv6 service"
	$(LD) -v $(LDOPTS) $(CRTI_OBJ) $(CRTBEGIN_OBJ) debug/test_ipv6.o $(LIBS_OBJ) $(CRTEND_OBJ) $(CRTN_OBJ) -o debug/$@

test: test_service

# OS libraries
###################################################
# OS
libc++abi.a: $(CXXABI_OBJ)
	ar rcs $@ $(CXXABI_OBJ)

os.a: $(OS_OBJECTS)
	@echo "\n>> Building the OS library"
	ar rcs $@ $(OS_OBJECTS)

install: #os.a bootloader
	@echo "\n>> Installing..."
	cp bootloader $(INSTALL)
	cp os.a $(INSTALL)
	cp libc++abi.a $(INSTALL)/libcxx
	cp -t$(INSTALL)/crt crt/crti.o crt/crtn.o
	cp -r ../api $(INSTALL)/
	cp linker.ld $(INSTALL)/
	@echo "\nDone!"

# Makefile recipes
###################################################
%.o: %.c
	$(CC) -MMD $(CCOPTS) -o $@ $< 

%.o: %.cpp 
	@echo "\n"
	$(CPP) -MMD $(CPPOPTS) -o $@ $< 

# AS-assembled object files
%.o: %.s
	@echo "\n"
	$(CPP) $(CPPOPTS) -x assembler-with-cpp $< -o $@

# Bootloader
###################################################
# Nasm-based bootloader
bootloader: boot/bootloader.asm
	@echo "\n>> Assembling bootloader"
	nasm -f bin -o bootloader $<

# Etags
###################################################
etags:
	@echo "\n>> Updating emacs tags "
	- find ../ -name "*.[hc]pp" | grep -v ".*\#.*" | etags -


# Cleanup
###################################################
clean_tests:
	$(RM) debug/*.o debug/*.d debug/*.img debug/test_service debug/test_ipv6

clean: clean_tests
	$(RM) $(OS_OBJECTS) $(CXXABI_OBJ) $(OS_DEPS)
	$(RM) os.a
	$(RM) bootloader	

-include $(OS_DEPS)

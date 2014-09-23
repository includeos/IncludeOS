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
INCLUDES = -I$(OSLIB)/include -I../abi/ -I./include
LIBS     = $(OSLIB)/lib
# 
CCOPTS	= -O2 -Wall -c -m32 -fno-stack-protector  -fno-builtin -march=i386 $(INCLUDES)  -lgcc

CPPOPTS = -std=c++11 -c -ffreestanding -nostdlib -fno-builtin -O2 -Wall -Wextra -fno-exceptions -fno-rtti $(INCLUDES) 

LDOPTS = -nostdlib -melf_i386 -N  --script=linker.ld 


# Objects
###################################################
TEST_OBJ= tests/tests.o

OS_OBJECTS=kernel_start.o syscalls.o interrupts.o class_os.o \
	hw/class_device.o hw/class_pci_device.o hw/class_nic.o\
	class_irq_handler.o \
	hw/pic.o  $(TEST_OBJ) virtio/virtionet.o virtio/virtio.o \
	class_pci_manager.o #hw/pci.o class_pci_manager_sanos.o 	#\


LIBC_OBJ = $(OSLIB)/lib/libc.a
LIBG_OBJ = $(OSLIB)/lib/libg.a 
LIBM_OBJ = $(OSLIB)/lib/libm.a 


# Complete bulid
###################################################
# A complete build includes:
# - a bootloader
# - an OS library for the service to link against
all: bootloader os.a

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
	@echo "\n>> Compiling the OS"
	ar rcs $@ $(OS_OBJECTS)

install: os.a
	@echo "\n>> Installing..."
	cp os.a $(OS)/lib
	cp bootloader $(OS)/bootloader
	mkdir -p $(OS)/crt
	cp crt/crt*.s $(OS)/crt
	cp -r ../abi $(OS)/abi
	cp linker.ld $(OS)
	@echo "\nDone!"

# Object files
###################################################
# General C++-files to object files. The OS is compiled here.
%.o: %.cpp %.h
	@echo "\n>> Compiling " $@
	$(CPP) $(CPPOPTS) -o $@ $< 

%.o: %.cpp
	@echo "\n>> Compiling " $@
	$(CPP) $(CPPOPTS) -o $@ $< 

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


# Cleanup
###################################################
clean: 
	$(RM) $(OS_OBJECTS)
	$(RM) $(SERVICE)*
	$(RM) os.a
	$(RM) bootloader





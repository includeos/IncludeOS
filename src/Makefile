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

CPPOPTS = -std=c++11 -c  -ffreestanding -nostdlib -fno-builtin -O2 -Wall -Wextra -fno-exceptions -fno-rtti $(INCLUDES) 

LDOPTS = -nostdlib -melf_i386 -N  --script=linker.ld 


# Objects
###################################################
TEST_OBJ= tests/tests.o

OS_OBJECTS=kernel_start.o syscalls.o interrupts.o class_os.o \
	hw/class_device.o hw/class_pci_device.o \
	class_irq_handler.o class_pci_manager.o \
	virtio/class_virtio.o virtio/class_virtionet.o hw/pic.o\
	  $(TEST_OBJ) #virtio/virtionet.o virtio/virtio.o	 \
	 #hw/pci.o class_pci_manager_sanos.o  hw/class_nic.o\ 

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
	sudo cp os.a $(OS)/lib
	sudo cp bootloader $(OS)/
	sudo mkdir -p $(OS)/crt
	sudo cp crt/crt*.s $(OS)/crt
	sudo cp -r ../abi $(OS)/
	sudo cp linker.ld $(OS)/
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

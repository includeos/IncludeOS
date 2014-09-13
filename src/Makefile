# Compiler/Linker
###################################################
CROSS = /usr/local/cross
CROSSLIB = $(CROSS)/i686-elf

# Options
###################################################
# Physical address of service / kernel
# - Needed by the linker to set appropriate offsets 
# - since our segment is 0x0, but our code is loaded at 0x8000
# - but, the text segment doesn't start until +0x74 due to elf-headers
# TODO: 
# - This should be inferred by inspeciting the elf-binary. 
# - ... but that's not ready until this makefile has run
SERVICE_LOCATION = 0x8080

DEBUG_OPTS = -ggdb3

CC  = $(CROSS)/bin/i686-elf-gcc #gcc
CPP = $(CROSS)/bin/i686-elf-g++ #g++
LD  = $(CROSS)/bin/i686-elf-ld #ld
INCLUDES = $(CROSSLIB)/include
LIBS     = $(CROSSLIB)/lib

# Optional options:
# -fno-defer-pop: 
#	- Force popping of args after each function call
#	- Necessary if you want to manually predict what's on stack
# -fstack-protector: 
#	- Put guard variables on stack, to identify stack overflows
#	- Here, we use the inverse, -fno-stack-protector, but it might be useful
# -To link with IncludeOS-ABI: -I../../IncludeOS-ABI
# 
CCOPTS	= -O2 -Wall -c -m32 -fno-stack-protector  -fno-builtin -march=i386 -I. -I$(INCLUDES) -lgcc
#-I ../../src/newlib-2.1.0/newlib/libc/include/ #-I../../dlmalloc

CPPOPTS = -std=c++11 -c -ffreestanding -nostdlib -fno-builtin -O2 -Wall -Wextra -fno-exceptions -fno-rtti -I. -I$(INCLUDES)
#-O2 -Wall -c -m32 -fno-stack-protector  -fno-builtin -march=i386 -I.  -std=c++11
LDOPTS = -nostdlib -melf_i386 -N  --script=linker.ld #  -Ttext $(SERVICE_LOCATION) -e '_start' --defsym _includeos=0xf0000 -nostartfiles


# Objects
###################################################


CRTI_OBJ=crti.o
CRTBEGIN_OBJ:=$(shell $(CPP) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CPP) -print-file-name=crtend.o)
CRTN_OBJ=crtn.o 

TEST_OBJ= tests/tests.o

SERVICE_OBJECTS=service.o # Feel free to add more...
OS_WRAP=crti.o crtn.o
OS_OBJECTS=kernel_start.o syscalls.o interrupts.o class_os.o \
	class_irq_handler.o class_pci_manager.o hw/pci.o hw/pic.o  $(TEST_OBJ) #


LIBC_OBJ = $(CROSSLIB)/lib/libc.a
LIBG_OBJ = $(CROSSLIB)/lib/libg.a 
LIBM_OBJ = $(CROSSLIB)/lib/libm.a 

# Full link list
OBJ_LIST:=$(CRTI_OBJ) $(CRTBEGIN_OBJ) \
	$(OS_OBJECTS) $(SERVICE_OBJECTS) $(LIBC_OBJ) $(LIBM_OBJ) \
	\$(CRTEND_OBJ) $(CRTN_OBJ)


# Complete bulid
###################################################
# A complete build includes:
# - a bootloader
# - a "service", to be linked with OS-objects (OS included)
# - a vmbuilder, to assemble the bootable image

#all: LDOPTS += -S #strip debug
all: bootloader vmbuilder service

stripped: LDOPTS += -s #strip all
stripped: bootloader vmbuilder service


# The same, but with debugging symbols (OBS: Dramatically increases binary size)
debug: CCOPTS  += $(DEBUG_OPTS)
debug: CPPOPTS += $(DEBUG_OPTS)
debug: LDOPTS  += -M --verbose

debug: OBJ_LIST += $(LIBG_OBJ)

debug: bootloader vmbuilder service #Don't wanna call 'all', since it strips debug info

# Service
###################################################
# - We want this as a special step since it's user dependent
service.o: service.cpp
	@echo "\n>> Compiling the service"
	$(CPP) $(CPPOPTS) -o $@ $<

# Link the service with the os
service: $(OS_WRAP) $(OS_OBJECTS) $(SERVICE_OBJECTS) 		
	@echo "\n>> Linking service with OS and finalizing"
	$(LD) $(LDOPTS) $(OBJ_LIST) -o $@


# C standard
###################################################
# malloc / newlib etc.
%.o: %.c %.h
	@echo "\n>>Building C libraries"
	$(CC) $(CCOPTS) -o $@ $<

#malloc.o: ../../newlib-2.1.0/newlib/libc/stdlib/malloc.c #../../dlmalloc/malloc.c
#	@echo "\n>>Building malloc.c"
#	$(CC) $(CCOPTS) -o $@ $<


# Object files
###################################################
# General C++-files to object files. The OS is compiled here.
%.o: %.cpp %.h
	@echo "\n>> Compiling OS-objects"
	$(CPP) $(CPPOPTS) -o $@ $< 

%.o: %.cpp
	@echo "\n>> Compiling OS object without header"
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
	nasm -f bin -g -l bootloader.lst -o bootloader bootloader.asm

# Optional, for gcc-assembled bootloader
bootloader_s: bootloader.s
	$(CC) $(CCOPTS) -o $@ $<

# VM builder
###################################################
# Compiling the vmbuilder, which becomes a host-executable directly
vmbuilder: vmbuilder.cpp
	@echo "\nCompiling the vmbulder"
	g++ -std=c++11 vmbuilder.cpp -o vmbuilder

# Cleanup
###################################################
clean: 
	-rm *.o
	-rm tests/*.o
	-rm hw/*.o
	-rm service
	-rm bootloader
	-rm vmbuilder
	-rm image


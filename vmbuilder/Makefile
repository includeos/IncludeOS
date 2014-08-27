# Objects & Options
###################################################


SERVICE_OBJECTS=service.o # Feel free to add more...

OS_START=kernel_start.o syscalls.o
OS_OBJECTS=class_os.o class_irq_handler.o 

# Physical address of service / kernel
# - Needed by the linker to set appropriate offsets 
# - since our segment is 0x0, but our code is loaded at 0x8000
# - but, the text segment doesn't start until +0x74 due to elf-headers
# TODO: 
# - This should be inferred by inspeciting the elf-binary. 
# - ... but that's not ready until this makefile has run
#SERVICE_LOCATION = 0x8000

DEBUG_OPTS= -g

# Compiler/Linker
###################################################
CC = /usr/local/cross/bin/i686-elf-gcc #gcc
CPP= /usr/local/cross/bin/i686-elf-g++ #g++
LD= /usr/local/cross/bin/i686-elf-ld #ld

# Optional options:
# -fno-defer-pop: 
#	- Force popping of args after each function call
#	- Necessary if you want to manually predict what's on stack
# -fstack-protector: 
#	- Put guard variables on stack, to identify stack overflows
#	- Here, we use the inverse, -fno-stack-protector, but it might be useful
# -To link with IncludeOS-ABI: -I../../IncludeOS-ABI
# 
CCOPTS	= -O2 -Wall -c -m32 -fno-stack-protector  -fno-builtin -march=i386 -I. -I/usr/local/cross/i686-elf/include/ 
#-I ../../src/newlib-2.1.0/newlib/libc/include/ #-I../../dlmalloc

CPPOPTS = -std=c++11 -c -ffreestanding -nostdlib -O2 -Wall -Wextra -fno-exceptions -fno-rtti -I. -I/usr/local/cross/i686-elf/include/
#-O2 -Wall -c -m32 -fno-stack-protector  -fno-builtin -march=i386 -I.  -std=c++11
LDOPTS = -nostartfiles -nostdlib -melf_i386 -N --script=linker.ld


# Complete bulid
###################################################
# A complete build includes:
# - a bootloader
# - a "service", to be linked with OS-objects (OS included)
# - a vmbuilder, to assemble the bootable image
all:  bootloader vmbuilder service


# The same, but with debugging symbols (OBS: Dramatically increases binary size)
debug: CCOPTS += $(DEBUG_OPTS)
debug: CPPOPTS += $(DEBUG_OPTS)
debug: all

# Service
###################################################
# - We want this as a special step since it's user dependent
service.o: service.cpp
	@echo "\n>> Compiling the service"
	$(CPP) $(CPPOPTS) -o $@ $<

# Link the service with the os
service: $(OS_START) $(OS_OBJECTS) $(SERVICE_OBJECTS) 		
	@echo "\n>> Linking service with OS and finalizing"
#-Ttext $(SERVICE_LOCATION)
	$(LD) $(LDOPTS)  $(OS_START) $(OS_OBJECTS) /usr/local/cross/i686-elf/lib/libc.a service.o -o service



# C standard
###################################################
# malloc / newlib etc.
%.o: %.c %.h
	@echo "\n>>Building C libraries"
	$(CC) $(CCOPTS) -o $@ $<

malloc.o: ../../newlib-2.1.0/newlib/libc/stdlib/malloc.c #../../dlmalloc/malloc.c
	$(CC) $(CCOPTS) -o $@ $<


# Object files
###################################################
# General C++-files to object files. The OS is compiled here.
%.o: %.cpp %.h
	@echo "\n>> Compiling OS-objects"
	$(CPP) $(CPPOPTS) -o $@ $< 

%.o: %.cpp
	@echo "\n>> Compiling OS object without header"
	$(CPP) $(CPPOPTS) -o $@ $< 

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
	rm *.o
	rm service
	rm bootloader
	rm vmbuilder


CC = gcc
GPP= g++
LD=ld

# Physical address of service / kernel
# - Needed by the linker to set appropriate offsets (since our segment is 0x0)
SERVICE_LOCATION    = 0x8074 #Was 8000, but it depends on the size of the pheader

#CCOPTS = -m32 -Wall -g -c -fno-builtin -fno-stack-protector -march=i386 \
	-DPROCESS_START=$(PROCESS_LOCATION)

CCOPTS	= -O2 -Wall -c -g -m32 -fno-stack-protector -fno-defer-pop -fno-builtin -march=i386 -I. -std=c++11
LDOPTS = -nostartfiles -nostdlib -melf_i386 -N

all: bootloader_asm service vmbuilder

service: service.cpp os class_os
	$(GPP) $(CCOPTS) service.cpp
	$(LD) $(LDOPTS)	-Ttext $(SERVICE_LOCATION) service.o class_os.o -o service

class_os: 
	$(GPP) $(CCOPTS) class_os.cpp

bootloader: bootloader.s
	$(CC) $(CCOPTS) -o $@ $<

bootloader_asm: bootloader.asm
	nasm -f bin -g -l bootloader.lst -o bootloader bootloader.asm

vmbuilder: vmbuilder.cpp
	g++ -std=c++11 vmbuilder.cpp -o vmbuilder


clean: 
	rm *.o
	rm service
	rm bootloader
	rm vmbuilder


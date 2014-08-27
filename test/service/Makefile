CCOPTS	= -O2 -Wall -c -m32 -fno-stack-protector -fno-defer-pop \
		-fno-builtin -march=i386 \
		-DKERNEL_ADDR=$(KERNEL_ADDR) \
		-DPROC1_ADDR=$(PROC1_ADDR) \
		-DPROC2_ADDR=$(PROC2_ADDR)

LDOPTS = -nostartfiles -nostdlib -melf_i386 -N

all:
	g++ -c $(CCOPTS) service.cpp


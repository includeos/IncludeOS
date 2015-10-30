# The name of your service
SERVICE = tcc_port
OPTIONS = -O2 -msse3 -Wall -Wextra -mstackrealign

# Modules
FILES = service.o

# Compiler/Linker
###################################################
OSDIR = /usr/local/IncludeOS
OSLIB = $(OSDIR)/i686-elf

DEBUG_OPTS = -ggdb3

CC  = $(OSDIR)/bin/i686-elf-gcc 
CPP = clang++-3.6 -target i686-elf
GPP = $(OSDIR)/bin/i686-elf-g++
LD  = $(OSDIR)/bin/i686-elf-ld 

INCLUDES = -I$(OSDIR)/api/sys -I$(OSLIB)/include -I$(OSDIR)/api -I$(OSDIR)/stdlib -Iinclude

CPPOPTS = -std=c++14 -c $(OPTIONS) -nostdlib -MMD -fno-stack-protector $(INCLUDES)
LDOPTS = -nostdlib -melf_i386 -N  --script=$(OSDIR)/linker.ld


# Objects
###################################################

GCC_VER = $(shell $(CC) -dumpversion)
LIBGCC = $(OSDIR)/lib/gcc/i686-elf/$(GCC_VER)/libgcc.a

LIBC_OBJ = $(OSLIB)/lib/libc.a
LIBG_OBJ = $(OSLIB)/lib/libg.a 
LIBM_OBJ = $(OSLIB)/lib/libm.a 

CRTBEGIN_OBJ:=$(shell $(GPP) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(GPP) -print-file-name=crtend.o)

# Full link list
OBJS = $(FILES:.cpp=.o) 
LIBS = libtcc.a libtcc1.a $(OSDIR)/lib/os.a $(OSDIR)/lib/libc++.a $(OSDIR)/lib/libc++abi.a $(OSDIR)/lib/os.a $(LIBC_OBJ) $(LIBM_OBJ) $(LIBGCC)

OS_PRE = $(CRTBEGIN_OBJ) crti.o
OS_POST = $(CRTEND_OBJ) crtn.o

DEPS = $(OBJS:.o=.d)

# Complete bulid
###################################################
# A complete build includes:
# - a "service", to be linked with OS-objects (OS included)

all: service

stripped: LDOPTS  += -S
stripped: CPPOPTS += -Oz
stripped: service


# The same, but with debugging symbols (OBS: Dramatically increases binary size)
debug: CCOPTS  += $(DEBUG_OPTS)
debug: CPPOPTS += $(DEBUG_OPTS)
debug: LDOPTS  += -M --verbose

debug: OBJS += $(LIBG_OBJ)

debug: service #Don't wanna call 'all', since it strips debug info

# Service
###################################################
service.o: service.cpp
	@echo "\n>> Compiling the service"
	$(CPP) $(CPPOPTS) -o $@ $<

# Link the service with the os
service: $(OBJS) $(LIBS) crti.o crtn.o
	@echo "\n>> Linking service with OS"
	$(LD) $(LDOPTS) $(OS_PRE) $(OBJS) $(LIBS) $(OS_POST) -o $(SERVICE)
	@echo "\n>> Building image " $(SERVICE).img
	$(OSDIR)/vmbuild $(OSDIR)/bootloader $(SERVICE)

# Object files
###################################################

# Runtime
crt%.o: $(OSDIR)/crt/crt%.s
	@echo "\n>> Assembling C runtime:" $@
	$(CPP) $(CPPOPTS) -x assembler-with-cpp $<

# General C++-files to object files
%.o: %.cpp
	$(CPP) $(CPPOPTS) -o $@ $< 

%.o: %.cc
	$(CPP) $(CPPOPTS) -o $@ $< 

# Cleanup
###################################################
clean: 
	$(RM) $(OBJS) $(DEPS) $(SERVICE) 
	$(RM) $(SERVICE).img $(SERVICE).img.vdi $(SERVICE).img.vmdk $(SERVICE).img.qcow2
	$(RM) crti.o crtn.o 
	$(RM) $(DEPS) crti.d crtn.d

-include $(DEPS)

# The name of your service
SERVICE = IncludeOS_tests
OPTIONS = -Ofast -msse3 -Wall -Wextra

# Modules
FILES = service.cpp

# Compiler/Linker
###################################################
OSDIR = /usr/local/IncludeOS
OSLIB = $(OSDIR)/i686-elf

DEBUG_OPTS = -ggdb3

CC  = $(OSDIR)/bin/i686-elf-gcc 
CPP = $(OSDIR)/bin/i686-elf-g++ 
LD  = $(OSDIR)/bin/i686-elf-ld 

INCLUDES = -I$(OSDIR)/abi -I$(OSDIR)/include -I$(OSDIR)/stdlib -I$(OSDIR)/mod/rapidjson/include

CPPOPTS = -std=c++11 -c $(OPTIONS) -MMD -ffreestanding -nostdlib -fno-builtin -fno-exceptions -fno-rtti $(INCLUDES)
LDOPTS = -nostdlib -melf_i386 -N  --script=$(OSDIR)/linker.ld
CCOPTS = -std=c11 -c $(OPTIONS) -MMD -ffreestanding -nostdlib -fno-builtin $(INCLUDES)


# Objects
###################################################

GCC_VER = $(shell $(CC) -dumpversion)
LIBGCC = $(OSDIR)/lib/gcc/i686-elf/$(GCC_VER)/libgcc.a

LIBC_OBJ = $(OSLIB)/lib/libc.a
LIBG_OBJ = $(OSLIB)/lib/libg.a 
LIBM_OBJ = $(OSLIB)/lib/libm.a 

CRTBEGIN_OBJ:=$(shell $(CPP) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CPP) -print-file-name=crtend.o)

# Full link list
OBJS  = $(FILES:.cpp=.o)
LIBS = $(OSDIR)/lib/os.a $(LIBC_OBJ) $(LIBM_OBJ) $(LIBGCC)

OS_PRE = $(CRTBEGIN_OBJ) crti.o
OS_POST = $(CRTEND_OBJ) crtn.o

DEPS = $(OBJS:.o=.d)

# Complete bulid
###################################################
# A complete build includes:
# - a "service", to be linked with OS-objects (OS included)

all: service

stripped: LDOPTS += -s #strip all
stripped: service


# The same, but with debugging symbols (OBS: Dramatically increases binary size)
debug: CCOPTS  += $(DEBUG_OPTS)
debug: CPPOPTS += $(DEBUG_OPTS)
debug: LDOPTS  += -M --verbose

debug: OBJS += $(LIBG_OBJ)

debug: service #Don't wanna call 'all', since it strips debug info

# Link the service with the os
service: $(OBJS) $(LIBS) crti.o crtn.o
	@echo "\n>> Linking service with OS"
	$(LD) $(LDOPTS) $(OS_PRE) $(OBJS) $(LIBS) $(OS_POST) -o $(SERVICE)
	objcopy --only-keep-debug $(SERVICE) $(SERVICE).gdb
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

# C to object files
%.o: %.c
	$(CC) $(CCOPTS) -o $@ $< 

# AS-assembled object files
%.o: %.s
	@echo "\n>> Assembling GNU 'as' files"
	$(CPP) $(CPPOPTS) -x assembler-with-cpp $<

# Cleanup
###################################################
clean: 
	$(RM) $(OBJS) crti.* crtn.* $(DEPS) $(SERVICE) $(SERVICE).img $(SERVICE).img.* $(SERVICE).vmdk $(SERVICE).vdi $(SERVICE).qcow2

-include $(DEPS)

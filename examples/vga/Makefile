# The name of your service
SERVICE = "IncludeOS_VGA"
OPTIONS = -Ofast -msse3 -Wall -Wextra

# Compiler/Linker
###################################################
OSDIR = /usr/local/IncludeOS
OSLIB = $(OSDIR)/i686-elf

DEBUG_OPTS = -ggdb3

CC  = $(OSDIR)/bin/i686-elf-gcc 
CPP = $(OSDIR)/bin/i686-elf-g++ 
LD  = $(OSDIR)/bin/i686-elf-ld 

INCLUDES = -I$(OSDIR)/abi/ -I./

LIBS     = $(OSLIB)/lib

CPPOPTS = -std=c++11 -c $(OPTIONS) -ffreestanding -nostdlib -fno-builtin -fno-exceptions -fno-rtti $(INCLUDES)
#-O2 -Wall -c -m32 -fno-stack-protector  -fno-builtin -march=i386 -I.  -std=c++11
LDOPTS = -nostdlib -melf_i386 -N  --script=$(OSDIR)/linker.ld #  -Ttext $(SERVICE_LOCATION) -e '_start' --defsym _includeos=0xf0000 -nostartfiles


# Objects
###################################################

CRTI_OBJ=crti.o
CRTBEGIN_OBJ:=$(shell $(CPP) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CPP) -print-file-name=crtend.o)
CRTN_OBJ=crtn.o 

OS_WRAP=crti.o crtn.o

TEST_OBJ= tests/tests.o

SERVICE_OBJECTS=service.o # Feel free to add more...


LIBC_OBJ = $(OSLIB)/lib/libc.a
LIBG_OBJ = $(OSLIB)/lib/libg.a 
LIBM_OBJ = $(OSLIB)/lib/libm.a 

# Full link list
OBJ_LIST:=$(CRTI_OBJ) $(CRTBEGIN_OBJ) \
	$(OSDIR)/lib/os.a $(SERVICE_OBJECTS) $(LIBC_OBJ) $(LIBM_OBJ) \
	\$(CRTEND_OBJ) $(CRTN_OBJ)


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

debug: OBJ_LIST += $(LIBG_OBJ)

debug: service #Don't wanna call 'all', since it strips debug info

# Service
###################################################
service.o: service.cpp
	@echo "\n>> Compiling the service"
	$(CPP) $(CPPOPTS) -o $@ $<

# Link the service with the os
service: $(OS_WRAP) $(SERVICE_OBJECTS) 	
	@echo "\n>> Linking service with OS"
	$(LD) $(LDOPTS) $(OBJ_LIST) -o $(SERVICE)
	@echo "\n>> Building image " $(SERVICE).img
	$(OSDIR)/vmbuild $(OSDIR)/bootloader $(SERVICE)

# Object files
###################################################

# Runtime
crt%.o: $(OSDIR)/crt/crt%.s
	@echo "\n>> Assembling C runtime:" $@
	$(CPP) $(CPPOPTS) -x assembler-with-cpp $<

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

# Cleanup
###################################################
clean: 
	$(RM) $(SERVICE_OBJECTS) $(SERVICE) $(OS_WRAP) $(SERVICE).img



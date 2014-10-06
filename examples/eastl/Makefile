# The name of your service
SERVICE = IncludeOS_eastl
OPTIONS = -Ofast -msse3 -Wall -Wextra

# Modules
SERV_OBJS = service.o

# Compiler/Linker
###################################################
OSDIR = /usr/local/IncludeOS
OSLIB = $(OSDIR)/i686-elf

DEBUG_OPTS = -ggdb3

CC  = $(OSDIR)/bin/i686-elf-gcc 
CPP = $(OSDIR)/bin/i686-elf-g++ 
LD  = $(OSDIR)/bin/i686-elf-ld 

INCLUDES = -I$(OSDIR)/abi -I$(OSDIR)/include -I$(OSDIR)/stdlib

CPPOPTS = -std=c++11 -c $(OPTIONS) -MMD -ffreestanding -nostdlib -fno-builtin -fno-exceptions -fno-rtti $(INCLUDES)
#-O2 -Wall -c -m32 -fno-stack-protector  -fno-builtin -march=i386 -I.  -std=c++11
LDOPTS = -nostdlib -melf_i386 -N  --script=$(OSDIR)/linker.ld #  -Ttext $(SERVICE_LOCATION) -e '_start' --defsym _includeos=0xf0000 -nostartfiles


# Objects
###################################################

LIBC_OBJ = $(OSLIB)/lib/libc.a
LIBG_OBJ = $(OSLIB)/lib/libg.a 
LIBM_OBJ = $(OSLIB)/lib/libm.a 

DIRS  = .
FILES = $(foreach dir, $(SOURCE_DIRS), $(dir)/*.cpp)
# Full link list
OBJS  = $(SERV_OBJS) $(FILES:.cpp=.o) crti.o crtn.o
LIBS = $(OSDIR)/lib/os.a $(LIBC_OBJ) $(LIBM_OBJ)

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

# Service
###################################################
service.o: service.cpp
	@echo "\n>> Compiling the service"
	$(CPP) $(CPPOPTS) -o $@ $<

# Link the service with the os
service: $(OBJS) $(LIBS)
	@echo "\n>> Linking service with OS"
	$(LD) $(LDOPTS) $(OBJS) $(LIBS) -o $(SERVICE)
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
	@echo "\n>> Compiling OS object without header"
	$(CPP) $(CPPOPTS) -o $@ $< 

# AS-assembled object files
%.o: %.s
	@echo "\n>> Assembling GNU 'as' files"
	$(CPP) $(CPPOPTS) -x assembler-with-cpp $<

# Cleanup
###################################################
clean: 
	$(RM) $(OBJS) $(DEPS) $(SERVICE) $(SERVICE).img

-include $(DEPS)

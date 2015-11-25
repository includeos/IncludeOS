#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = IncludeOS_Demo_Service

# Your service parts
FILES = service.cpp

# IncludeOS location
ifndef INCLUDEOS_INSTALL
	INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Shorter name
INSTALL = $(INCLUDEOS_INSTALL)

# Compiler/Linker
###################################################

OPTIONS = -Ofast -msse3 -Wall -Wextra -mstackrealign 

# External Libraries
###################################################
LIBC_OBJ = $(INSTALL)/newlib/libc.a
LIBG_OBJ = $(INSTALL)/newlib/libg.a
LIBM_OBJ = $(INSTALL)/newlib/libm.a 

LIBGCC = $(INSTALL)/libgcc/libgcc.a
LIBCXX = $(INSTALL)/libcxx/libc++.a $(INSTALL)/libcxx/libc++abi.a


INC_NEWLIB=$(INSTALL)/newlib/include
INC_LIBCXX=$(INSTALL)/libcxx/include

DEBUG_OPTS = -ggdb3 -v

CPP = clang++-3.6 -target i686-elf
LD  = ld 

INCLUDES = -I$(INC_LIBCXX) -I$(INSTALL)/api/sys -I$(INC_NEWLIB) -I$(INSTALL)/api 

CAPABS_COMMON = -msse3 -mstackrealign # Needed for 16-byte stack alignment (SSE)

all: CAPABS  =  $(CAPABS_COMMON) -O2  
debug: CAPABS = $(CAPABS_COMMON) -O0
stripped: CAPABS = $(CAPABS_COMMON) -Oz

WARNS   = -Wall -Wextra #-pedantic
CPPOPTS = $(CAPABS) $(WARNS) -c -m32 -std=c++14 -fno-stack-protector $(INCLUDES) -D_LIBCPP_HAS_NO_THREADS=1 #-flto -fno-exceptions

LDOPTS = -nostdlib -melf_i386 -N  --script=$(INSTALL)/linker.ld -flto


# Objects
###################################################

CRTBEGIN_OBJ = $(INSTALL)/crt/crtbegin.o
CRTEND_OBJ = $(INSTALL)/crt/crtend.o
CRTI_OBJ = $(INSTALL)/crt/crti.o
CRTN_OBJ = $(INSTALL)/crt/crtn.o

# Full link list
OBJS  = $(FILES:.cpp=.o) .service_name.o
LIBS =  $(INSTALL)/os.a $(LIBCXX) $(INSTALL)/os.a $(LIBC_OBJ) $(LIBM_OBJ) $(LIBGCC)

OS_PRE = $(CRTBEGIN_OBJ) $(CRTI_OBJ)
OS_POST = $(CRTEND_OBJ) $(CRTN_OBJ)

DEPS = $(OBJS:.o=.d)

# Complete bulid
###################################################
# A complete build includes:
# - a "service", to be linked with OS-objects (OS included)

all: service

stripped: LDOPTS  += -S #strip all
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

.service_name.o: $(INSTALL)/service_name.cpp
	$(CPP) $(CPPOPTS) -DSERVICE_NAME="\"$(SERVICE)\"" -o $@ $<

# Link the service with the os
service: $(OBJS) $(LIBS) 
	@echo "\n>> Linking service with OS"
	$(LD) $(LDOPTS) $(OS_PRE) $(OBJS) $(LIBS) $(OS_POST) -o $(SERVICE)
	@echo "\n>> Building image " $(SERVICE).img
	$(INSTALL)/vmbuild $(INSTALL)/bootloader $(SERVICE)

# Object files
###################################################

# Runtime
crt%.o: $(INSTALL)/crt/crt%.s
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
	$(RM) $(OBJS) $(DEPS) $(SERVICE) 
	$(RM) $(SERVICE).img

-include $(DEPS)

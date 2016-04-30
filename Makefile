#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = Acorn
SERVICE_NAME = My Acorn Service

# Your service parts
FILES = service.cpp memdisk.o

# Your disk image
DISK=

# Your own include-path
LOCAL_INCLUDES=-I./http/inc

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

all: memdisk.o


# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed


memdisk.o: memdisk.asm
	@echo "\n>> Assembling memdisk"
	nasm -f elf -o memdisk.o $<

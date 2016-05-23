#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = Acorn
SERVICE_NAME = Acorn

# Your service parts
FILES = service.cpp memdisk.o server/request.o server/response.o server/connection.o server/server.o server/http/uri/uri.o

# Your disk image
DISK=

# Modules
CUSTOM_MODULES =-I./bucket -I./json
MOD_FILES =

FILES += $(MOD_FILES)

# Your own include-path
LOCAL_INCLUDES=$(CUSTOM_MODULES) -I./server -I./server/http/uri -I./server/http/inc -I./rapidjson/include

# Local target dependencies
#.PHONY: memdisk.fat
all: memdisk.fat memdisk.o server/router.hpp server/server.hpp

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed


# Local targets

# Create memdisk.fat from ./memdisk contents
memdisk.fat:
	@echo "\n>> Creating memdisk image from directory ./memdisk"
	$(INCLUDEOS_INSTALL)/etc/create_memdisk.sh

# Assemble memdisk.fat into an elf-binary, memdisk.o to link with the service
memdisk.o: memdisk.asm
	@echo "\n>> Assembling memdisk"
	nasm -f elf -o memdisk.o $<

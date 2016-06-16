#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = Acorn
SERVICE_NAME = Acorn

# Your service parts
FILES = service.cpp server/request.o server/response.o server/connection.o server/server.o server/http/uri/uri.o

# Your disk image
DISK=memdisk.fat

# Modules
CUSTOM_MODULES =-I./bucket -I./json
MOD_FILES =

FILES += $(MOD_FILES)

# Your own include-path
LOCAL_INCLUDES=$(CUSTOM_MODULES) -I./server -I./server/http/uri -I./server/http/inc -I./rapidjson/include

# Local target dependencies
#.PHONY: memdisk.fat
all: server/router.hpp server/server.hpp

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed

disk:
	rm -f memdisk.fat
	make


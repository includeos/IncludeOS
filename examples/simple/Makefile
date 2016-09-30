#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = example_simple
SERVICE_NAME = Mana - Simple Example

# Your service parts
FILES = service.cpp

# Your disk image
DISK=

# Additional drivers (virtionet, virtioblk etc)
DRIVERS=virtionet

ROOT=../..
# Your own include-path
LOCAL_INCLUDES=-I$(ROOT)/include -I$(ROOT)/lib/http/uri/include

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

all: build_mana

build_mana:
	$(MAKE) -C ../../

EXTRA_LIBS+=$(ROOT)/libmana.a
# TODO: Avoid including here
EXTRA_LIBS+=$(ROOT)/lib/http/uri/liburi.a

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed

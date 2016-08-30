#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_platform_unik
SERVICE_NAME = Test Unik platform initialization

# Your service parts
FILES = service.cpp

LOCAL_INCLUDES = -I$(INCLUDEOS_SRC)/test/lest/include/lest

# Your disk image
DISK=

DRIVERS=virtionet

PLATFORM=unik

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

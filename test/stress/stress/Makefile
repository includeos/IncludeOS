#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_stresstest
SERVICE_NAME = IncludeOS stress test

# Your service parts
FILES = service.cpp

LOCAL_INCLUDES=-I$(INCLUDEOS_SRC)/test/lest/include/lest

# Your disk image
DISK=

# Add network driver
DRIVERS=virtionet

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

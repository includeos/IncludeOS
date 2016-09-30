#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_stresstest
SERVICE_NAME = IncludeOS stress test

# Your service parts
FILES = service.cpp

# IncludeOS_SRC location
ifndef INCLUDEOS_SRC
current_dir := $(shell pwd)
LOCAL_INCLUDES=-I$(abspath $(current_dir)/../../../lest/include/lest)
else
LOCAL_INCLUDES=-I$(INCLUDEOS_SRC)/test/lest/include/lest
endif

# Your disk image
DISK=

# Add network driver
DRIVERS=virtionet

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

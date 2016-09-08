#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_exceptions
SERVICE_NAME = Exceptions test

# Your service parts
FILES = service.cpp

# Your disk image
DISK=

# IncludeOS_SRC location
ifndef INCLUDEOS_SRC
current_dir := $(shell pwd)
LOCAL_INCLUDES=-I$(abspath $(current_dir)/../../../lest/include/lest)
else
LOCAL_INCLUDES=-I$(INCLUDEOS_SRC)/test/lest/include/lest
endif

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

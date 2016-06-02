#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_STL
SERVICE_NAME = IncludeOS basic STL test

# Your service parts
FILES = service.cpp

LOCAL_INCLUDES=-I$(shell PWD)/../lest/include/lest

# Your disk image
DISK=

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

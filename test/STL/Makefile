#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = Test_STL
SERVICE_NAME = IncludeOS basic STL test

# Your service parts
FILES = service.cpp

LOCAL_INCLUDES=-I$(PWD)/../lest/include/lest

# Your disk image
DISK=

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

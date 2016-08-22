#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_tcp
SERVICE_NAME = TCP Test Service

# Your service parts
FILES = service.cpp
# Your disk image
DISK=
LOCAL_INCLUDES=

# Add network driver
DRIVERS=virtionet

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

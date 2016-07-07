#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_memdisk
SERVICE_NAME = The Memdisk Test Service

# Your service parts
FILES=nosector.cpp
# Your disk image
DISK=sector0.disk

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

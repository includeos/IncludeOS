#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_dhcp
SERVICE_NAME = IncludeOS DHCP test

# Your service parts
FILES = service.cpp

# Your disk image
DISK=

# Add network driver
DRIVERS=virtionet

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

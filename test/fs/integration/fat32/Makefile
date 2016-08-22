#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_fat32
SERVICE_NAME = FAT32 Test Service

# Your service parts
FILES=fat32.cpp
# Your disk image
DISK=

# Add virtioblk driver
DRIVERS=virtioblk

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

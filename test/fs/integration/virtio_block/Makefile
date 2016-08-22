#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_virtio_block
SERVICE_NAME = Virtio Block Test Service

# Your service parts
FILES=service.cpp

# Your disk image
DISK=

# Add virtioblk driver
DRIVERS=virtioblk

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

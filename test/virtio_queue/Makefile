#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_virtio_queue
SERVICE_NAME = Virtio Queue Test

# Your service parts
FILES = service.cpp

LOCAL_INCLUDES=-I$(PWD)/../lest/include/lest -I$(PWD)/../../mod/GSL/include

# Your disk image
DISK=



# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

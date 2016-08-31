#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = tcp_demo
SERVICE_NAME = IncludeOS TCP Demo

# Your service parts
FILES = service.cpp

# Your disk image
DISK=

DRIVERS=virtionet

# Your own include-path
LOCAL_INCLUDES=

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed

#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = IncludeOS_Demo_Service
SERVICE_NAME = IncludeOS Demo Service

# Your service parts
FILES = service.cpp

# Your disk image
DISK=

# Your own include-path
LOCAL_INCLUDES=

# Add networking driver
DRIVERS=virtionet

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed

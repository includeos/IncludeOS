#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = LiveUpdate
SERVICE_NAME = Live Update Test Service

# Your service parts
FILES = service.cpp storage.cpp update.cpp resume.cpp hotswap.o

# Your disk image
DISK=

# We need to control memory for testing purposes
MAX_MEM=128

#
DRIVERS=virtionet

# Your own include-path
LOCAL_INCLUDES=

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed

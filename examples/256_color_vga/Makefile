#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = 256_Color_VGA
SERVICE_NAME = 256 Color VGA

# Your service parts
FILES = service.o int32.o

# Your disk image
DISK=

DRIVERS=

# Your own include-path
LOCAL_INCLUDES=

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed

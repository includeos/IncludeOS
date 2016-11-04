#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_posix_syslog
SERVICE_NAME = POSIX Syslog Test Service

# Your service parts
FILES = service.cpp

# Your disk image
DISK=

# Additional drivers (virtionet, virtioblk etc)
DRIVERS=virtionet

# Your own include-path
LOCAL_INCLUDES=

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed

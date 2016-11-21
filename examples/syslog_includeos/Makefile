#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = syslog_includeos
SERVICE_NAME = Syslog IncludeOS Service (Plugin)

# Your service parts
FILES = service.cpp

# Your disk image
DISK=

# Additional drivers (virtionet, virtioblk etc)
DRIVERS=virtionet

# Your own include-path
LOCAL_INCLUDES=

PLUGIN=syslogd

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed

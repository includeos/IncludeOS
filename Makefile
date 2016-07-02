#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = IRCd
SERVICE_NAME = IncludeOS IRC Service

# Your service parts
FILES = service.cpp ircd.cpp client.cpp client_new.cpp client_msg.cpp channel.cpp umodes.cpp cmodes.cpp transform.cpp

# Your disk image
DISK=

# Your own include-path
LOCAL_INCLUDES=

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed

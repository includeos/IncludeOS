#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_exceptions
SERVICE_NAME = Exceptions test

# Your service parts
FILES = service.cpp

# Your disk image
DISK=

LOCAL_INCLUDES=-I$(INCLUDEOS_SRC)/test/lest/include/lest

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

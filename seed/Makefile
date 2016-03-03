#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = My_Service
SERVICE_NAME = "My IncludeOS Service Name"

# Your service parts
FILES = service.cpp
# Your disk image
DISK=

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Shorter name
INSTALL = $(INCLUDEOS_INSTALL)

all:
	$(MAKE) -f $(INSTALL)/Makeseed all FILES=$(FILES) SERVICE=$(SERVICE) SERVICE_NAME=$(SERVICE_NAME) DISK=$(DISK)

stripped:
	$(MAKE) -f $(INSTALL)/Makeseed stripped FILES=$(FILES) SERVICE=$(SERVICE) SERVICE_NAME=$(SERVICE_NAME) DISK=$(DISK)

# Cleanup
###################################################
clean:
		$(MAKE) -f $(INSTALL)/Makeseed clean

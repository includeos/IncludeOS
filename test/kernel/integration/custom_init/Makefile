#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_custom_init
SERVICE_NAME = Custom pre-service initialization

# Your service parts
FILES = service.cpp custom_init1.cpp custom_init2.cpp custom_init3.cpp

# IncludeOS_SRC location
ifndef INCLUDEOS_SRC
current_dir := $(shell pwd)
LOCAL_INCLUDES=-I$(abspath $(current_dir)/../../../lest/include/lest)
else
LOCAL_INCLUDES=-I$(INCLUDEOS_SRC)/test/lest/include/lest
endif

# Your disk image
DISK=

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

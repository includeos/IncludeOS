#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# The name of your service
SERVICE = test_GSL
SERVICE_NAME = C++ Core Guideline Support Library Tests

# Your service parts
FILES = service.cpp

LOCAL_INCLUDES=-I$(INCLUDEOS_SRC)/mod/GSL/include -I$(INCLUDEOS_SRC)/test/lest/include/lest

# IncludeOS_SRC location
ifndef INCLUDEOS_SRC
current_dir := $(shell pwd)
LOCAL_INCLUDES=-I$(abspath $(current_dir)/../../../lest/include/lest) -I$(abspath $(current_dir)/../../../../mod/GSL/include)
else
LOCAL_INCLUDES=-I$(INCLUDEOS_SRC)/test/lest/include/lest -I$(INCLUDEOS_SRC)/mod/GSL/include
endif

# Your disk image
DISK=

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

include $(INCLUDEOS_INSTALL)/Makeseed

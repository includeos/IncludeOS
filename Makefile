#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# Service name
SERVICE=Acorn
SERVICE_NAME=Acorn

# Service parts
FILES=service.cpp server/request.o server/response.o server/connection.o server/server.o\
      cookie/cookie.o cookie/cookie_jar.o route/path_to_regex.o middleware/waitress.o

# Service disk image
DISK=memdisk.fat

# Service modules
CUSTOM_MODULES=-I./app -I./bucket -I./cookie -I./json -I./middleware -I./stats -I/route
MOD_FILES=

FILES+=$(MOD_FILES)

# Add network driver
DRIVERS=virtionet

# Paths to interfaces
LOCAL_INCLUDES=$(CUSTOM_MODULES) -I. -I./app/routes -I./server -I./server/http/uri/include -I./server/http/inc -I./rapidjson/include #-DVERBOSE_WEBSERVER

# Local target dependencies
#.PHONY: memdisk.fat
all: build_uri server/router.hpp server/server.hpp

build_uri:
	$(MAKE) -C server/http/uri

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed

LIBS+=server/http/uri/liburi.a

disk:
	rm -f memdisk.fat
	make

clean: clean_uri

clean_uri:
	$(MAKE) -C server/http/uri clean

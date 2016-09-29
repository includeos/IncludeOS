#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# Service name
SERVICE=Acorn
SERVICE_NAME=Acorn

# Service parts
FILES=service.cpp logger/logger.o fs/acorn_fs.o

# Service disk image
DISK=memdisk.fat

# Service modules
CUSTOM_MODULES=-I./app -I./app/routes -I./lib -I./fs -I./test/lest/include
LIB_INCLUDES = -I./lib/mana/include -I./lib/mana/lib/http/uri/include -I./lib/mana/lib/http/inc -I./lib/dashboard/include

MOD_FILES=lib/cookie/cookie.o lib/cookie/cookie_jar.o lib/butler/butler.o lib/director/director.o \
      lib/dashboard/src/dashboard.o

FILES+=$(MOD_FILES)

# Add network driver
DRIVERS=virtionet

# Paths to interfaces
LOCAL_INCLUDES=-I. $(CUSTOM_MODULES) $(LIB_INCLUDES) #-DVERBOSE_WEBSERVER

# Local target dependencies
#.PHONY: memdisk.fat
all: mana

mana:
	$(MAKE) -C lib/mana

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makeseed

HEST := lib/mana/libmana.a lib/mana/lib/http/uri/liburi.a $(LIBS)

LIBS = $(HEST)

disk:
	rm -f memdisk.fat
	make

clean: clean_mana

clean_mana:
	$(MAKE) -C lib/mana clean

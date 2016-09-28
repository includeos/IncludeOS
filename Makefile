#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# Service name
SERVICE=Acorn
SERVICE_NAME=Acorn

# Service parts
FILES=lib/cookie/cookie.o lib/cookie/cookie_jar.o lib/butler/butler.o lib/director/director.o \
      lib/dashboard/src/dashboard.o logger/logger.o fs/acorn_fs.o

# Service disk image
DISK=memdisk.fat

# Service modules
CUSTOM_MODULES=-I./app -I./bucket -I./middleware -I/route
CUSTOM_MODULES+=-I./fs
MOD_FILES=

FILES+=$(MOD_FILES)

# Add network driver
DRIVERS=virtionet

# Paths to interfaces
LOCAL_INCLUDES=$(CUSTOM_MODULES) -I. -I./app/routes -I./lib -I./lib/mana/include -I./lib/mana/lib/http/uri/include -I./lib/mana/lib/http/inc -I./rapidjson/include #-DVERBOSE_WEBSERVER

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

LIBS += lib/mana/libmana.a lib/mana/lib/http/uri/liburi.a

disk:
	rm -f memdisk.fat
	make

clean: clean_mana

clean_mana:
	$(MAKE) -C lib/mana clean

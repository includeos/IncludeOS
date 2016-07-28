#################################################
#          IncludeOS vmbuilder makefile         #
#################################################

CPP = $(shell command -v clang++-3.8 || command -v clang++-3.6)
CPPOPTS = -std=c++11 -c -Wall -Wextra -O3 -I.
OBJS = vmbuild.o
OUT = vmbuild

ifndef INCLUDEOS_INSTALL
	INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

.cpp.o:
	@ $(CPP) $(CPPOPTS) $< -o $@

all: $(OBJS)
	@ echo ">>> Building vmbuilder"
	@ $(CPP) $(OBJS) -o $(OUT)
	@ echo "  * Done \n"

install:
	cp $(OUT) $(INCLUDEOS_INSTALL)

clean:
	$(RM) $(OBJS) $(OUT) *~

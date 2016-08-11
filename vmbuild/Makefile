#################################################
#          IncludeOS vmbuilder makefile         #
#################################################

ifndef INCLUDEOS_INSTALL
	INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

CPP = $(shell command -v clang++-3.8 || command -v clang++-3.6 || command -v clang++)
CPPOPTS = -std=c++14 -c -Wall -Wextra -O3  -I$(INCLUDEOS_INSTALL)/api -I$(INCLUDEOS_INSTALL)/mod/GSL -I.
OBJS = vmbuild.o elf_binary.o
OUT = vmbuild

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

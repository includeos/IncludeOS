#################################################
#          IncludeOS vmbuilder makefile         #
#################################################

ifndef INCLUDEOS_INSTALL
	INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

CPP = $(shell command -v clang++-3.8 || command -v clang++-3.6 || command -v clang++)
CPPOPTS = -std=c++14 -c -Wall -Wextra -O2
INCLUDES = -I$(INCLUDEOS_INSTALL)/api -I$(INCLUDEOS_INSTALL)/mod/GSL
OBJS = vmbuild.o elf_binary.o
OUT = vmbuild

%.o: %.cpp
	$(CPP) $(CPPOPTS) $(INCLUDES) $< -o $@

.PHONY: vmbuild elfsyms

all: vmbuild elfsyms

vmbuild: $(OBJS)
	@ echo ">>> Building vmbuilder"
	$(CPP) $(OBJS) -o $(OUT)

elfsyms: elf_syms.o
	$(CPP) -std=c++14 elf_syms.o -o elf_syms

install: vmbuild elfsyms
	cp $(OUT) $(INCLUDEOS_INSTALL)
	cp elf_syms $(INCLUDEOS_INSTALL)

clean:
	$(RM) $(OBJS) $(OUT) *~
	$(RM) elf_syms.o elf_syms

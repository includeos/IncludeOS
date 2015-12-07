ifndef CPP_INC
	CPP_INC=clang++-3.6
endif
CPPOPTS = -std=c++11 -c -Wall -Wextra -O3
OBJS = vmbuild.o
OUT = vmbuild

.cpp.o:
	@ $(CPP_INC) $(CPPOPTS) $< -o $@

all: $(OBJS)
	@ echo ">>> Building vmbuilder"
	@ $(CPP_INC) $(OBJS) -o $(OUT)
	@ echo "  * Done \n"

clean:
	$(RM) $(OBJS) $(OUT) *~




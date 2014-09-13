CPP=g++
CPPOPTS = -std=c++11 -c -Wall -Wextra -O3
OBJS = buildvm.o
OUT = buildvm

.cpp.o:
	$(CPP) $(CPPOPTS) $< -o $@

all: $(OBJS)
	@ echo ">>> Building buildvm"
	@ $(CPP) $(OBJS) -o $(OUT)
	@ echo "  * Done \n"

clean:
	$(RM) $(OBJS) $(OUT)
	$(RM) *~

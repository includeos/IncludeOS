OUTPUT=./test.lest

CXX   = g++ -std=c++11
DEFS  = -DHAVE_LEST_MAIN -Dlest_FEATURE_AUTO_REGISTER=1
INC   = -Ilest/include -I../api -Itest

FILES  = $(shell find -path ./lest -prune -o -wholename '*/unit/*.cpp' -printf '%p ')
FILES += test/lestmain.cxx

all: $(OBJS)
	$(CXX) $(INC) $(DEFS) $(FILES) -o $(OUTPUT)
	$(RM) $(OUTPUT)

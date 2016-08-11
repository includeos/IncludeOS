OUTPUT=./test.lest

CXX   = g++ -std=c++14
DEFS  = -Dlest_FEATURE_AUTO_REGISTER=1
INC   = -Ilest/include -I../api -I../mod/GSL/include -Itest

FILES  = $(shell find -path ./lest -prune -o -wholename '*/unit/*.cpp' -printf '%p ')
FILES += test/lestmain.cxx

all: $(OBJS)
	$(CXX) $(INC) $(DEFS) -DHAVE_LEST_MAIN $(FILES) -o $(OUTPUT)
	$(OUTPUT)
	$(RM) $(OUTPUT)

single: $(OBJS)
	$(CXX) $(INC) $(DEFS) $(FILES) -o $(OUTPUT)
	$(OUTPUT)
	$(RM) $(OUTPUT)

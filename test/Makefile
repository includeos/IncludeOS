OUTPUT=./test.lest

ifndef CXX
	CXX = g++
endif

CAPABS = -std=c++14
DEFS  = -Dlest_FEATURE_AUTO_REGISTER=1

INC   = -Ilest/include -I../api -I../api/posix -I../mod/GSL -Ilest_util -I../src

FILES  = $(shell find -path ./lest -prune -o -wholename '*/unit/*.cpp' -printf '%p ')
FILES += lest_util/lestmain.cxx

all: $(OBJS)
	@echo "Building all unit tests..."
	$(CXX) $(CAPABS) $(INC) $(DEFS) -DHAVE_LEST_MAIN $(FILES) -o $(OUTPUT)

single: $(OBJS)
	$(CXX) $(CAPABS) $(INC) $(DEFS) $(FILES) -o $(OUTPUT)

run:
	$(OUTPUT)

clean:
	$(RM) $(OUTPUT)

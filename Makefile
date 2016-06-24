# This file is a part of the IncludeOS unikernel - www.includeos.org
#
# Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
# and Alfred Bratterud
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#################################################
#          IncludeOS LIBRARY makefile           #
#################################################

# The name of your library
LIBRARY = liburi.a

# Your library parts
FILES = ${wildcard src/*.cpp}

# Your own include-path
LOCAL_INCLUDES=-I./include -I./GSL/include

# IncludeOS location
ifndef INCLUDEOS_INSTALL
INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

# Include the installed seed makefile
include $(INCLUDEOS_INSTALL)/Makelib


lib: ${OBJECTS}
	mkdir -p lib
	ar -cq $(LIB) ${OBJECTS}
	ranlib $(LIB)

test: test.cpp lib
	${CXX} ${CXXFLAGS} ${INCLUDES} -o test test.cpp $(LIB)

install:
	mkdir -p ${INCLUDEOS_INSTALL}/packages/lib/
	cat include/*.hpp > ${INCLUDEOS_INSTALL}/packages/include/$(NAME)
	cp -r lib/* ${INCLUDEOS_INSTALL}/packages/lib/
	cp -r GSL/include/* ${INCLUDEOS_INSTALL}/packages/include/

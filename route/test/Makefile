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

CXX=$(shell command -v clang++ || command -v clang++-3.8 || command -v clang++-3.7 || command -v clang++-3.6)
CXXFLAGS=-std=c++14 -Ofast
SRC=../path_to_regex.cpp

all: path_to_regex_parse path_to_regex_options path_to_regex_no_options

path_to_regex_parse: path_to_regex_parse.cpp
	${CXX} ${CXXFLAGS} -opath_to_regex_parse path_to_regex_parse.cpp ${SRC}

path_to_regex_options: path_to_regex_options.cpp
	${CXX} ${CXXFLAGS} -opath_to_regex_options path_to_regex_options.cpp ${SRC}

path_to_regex_no_options: path_to_regex_no_options.cpp
	${CXX} ${CXXFLAGS} -opath_to_regex_no_options path_to_regex_no_options.cpp ${SRC}

clean:
	rm -f path_to_regex_parse
	rm -f path_to_regex_options
	rm -f path_to_regex_no_options

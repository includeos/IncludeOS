#!/bin/bash
set -e
clang++-3.8 -std=c++11 verify.cpp ../IncludeOS/src/util/crc32.cpp -I../IncludeOS/api -o verify
./verify
rm -f verify

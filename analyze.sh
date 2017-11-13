#!/bin/bash
INC="-Iapi/posix -Isrc/include -I$INCLUDEOS_PREFIX/includeos/x86_64/include/libcxx -I$INCLUDEOS_PREFIX/includeos/x86_64/include/newlib -Iapi -Imod -Imod/GSL -Imod/rapidjson/include -Imod/uzlib/src"
DEF="-DNO_DEBUG=1 -DOS_TERMINATE_ON_CONTRACT_VIOLATION -D_GNU_SOURCE -D_LIBCPP_HAS_NO_THREADS=1 -DOS_VERSION=1 -DARCH_x86_64 -DARCH=x86_64"
CHK="clang-analyzer-*,bugprone-*,modernize-*,performance-*,misc-*"

clang-tidy-5.0 -checks=$CHK `find src -name '*.cpp'` -- -nostdlib -nostdinc -std=c++14 $INC $DEF

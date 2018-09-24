#!/bin/bash
set -e

# Install the IncludeOS libraries (i.e. IncludeOS_home) from binary bundle
# ...as opposed to building them all from scratch, which takes a long time
#
#
# OPTIONS:
#
# Location of the IncludeOS repo:
# $ export INCLUDEOS_SRC=your/github/cloned/IncludeOS
#
# Parent directory of where you want the IncludeOS libraries (i.e. IncludeOS_home)
# $ export INCLUDEOS_PREFIX=parent/folder/for/IncludeOS/libraries i.e.

: ${INCLUDEOS_SRC:=$HOME/IncludeOS}
: ${INCLUDEOS_PREFIX:=/usr/local}
: ${INCLUDEOS_ENABLE_TEST:=OFF}
: ${num_jobs:="-j 4"}
: ${ARCH:=x86_64}

# Set compiler version
source $INCLUDEOS_SRC/etc/use_clang_version.sh
echo -e "\n\n>>> Best guess for compatible compilers: $CXX / $CC"

# Build IncludeOS
echo -e "\n\n>>> Building IncludeOS"
mkdir -p $INCLUDEOS_SRC/build_${ARCH}
pushd $INCLUDEOS_SRC/build_${ARCH}
cmake $INCLUDEOS_SRC \
	  -DCMAKE_INSTALL_PREFIX=$INCLUDEOS_PREFIX \
	  -Dtests=$INCLUDEOS_ENABLE_TEST \
    -Dthreading=$INCLUDEOS_THREADING \
	  -DBUNDLE_LOC=$BUNDLE_LOC
make PrecompiledLibraries
make $num_jobs
make install
popd

echo -e "\n\n>>> Done! IncludeOS bundle downloaded and installed"

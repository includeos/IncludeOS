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

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}
INCLUDEOS_PREFIX=${INCLUDEOS_PREFIX-/usr/local}

# Try to find suitable compiler
cc_list="clang-3.9 clang-3.8 clang-3.7 clang-3.6 clang"
cxx_list="clang++-3.9 clang++-3.8 clang++-3.7 clang++-3.6 clang++"

compiler=""
guess_compiler() {
    for compiler in $*
    do
	if command -v $compiler; then
		break
	fi
    done
}


if [ -z "$CC" ]
then
	guess_compiler "$cc_list"
	export CC=$compiler
fi

if [ -z "$CXX" ]
then
	guess_compiler "$cxx_list"
	export CXX=$compiler
fi

echo -e "\n\n>>> Best guess for compatible compilers: $CXX / $CC"

# Build IncludeOS
echo -e "\n\n>>> Building IncludeOS"
mkdir -p $INCLUDEOS_SRC/build
pushd $INCLUDEOS_SRC/build
cmake $INCLUDEOS_SRC -DCMAKE_INSTALL_PREFIX=$INCLUDEOS_PREFIX -Dtests=$INCLUDEOS_ENABLE_TEST
make PrecompiledLibraries
make -j 4
make install
popd

echo -e "\n\n>>> Done! IncludeOS bundle downloaded and installed"

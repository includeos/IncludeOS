#!/bin/bash

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
# $ export INCLUDEOS_INSTALL_LOC=parent/folder/for/IncludeOS/libraries i.e.

[ ! -v INCLUDEOS_SRC ] && export INCLUDEOS_SRC=$(readlink -f "$(dirname "$0")/")
[ ! -v INCLUDEOS_INSTALL_LOC ] && export INCLUDEOS_INSTALL_LOC=$HOME
export INCLUDEOS_HOME=$INCLUDEOS_INSTALL_LOC/IncludeOS_install

# Install dependencies
. $INCLUDEOS_SRC/etc/prepare_ubuntu_deps.sh

DEPENDENCIES="curl make clang-$clang_version nasm bridge-utils qemu"
echo ">>> Installing dependencies (requires sudo):"
echo "    Packages: $DEPENDENCIES"
sudo apt-get update
sudo apt-get install -y $DEPENDENCIES

echo -e "\n\n>>> Calling install_from_bundle.sh script"
$INCLUDEOS_SRC/etc/install_from_bundle.sh


echo -e "\n\n>>> Creating a virtual network, i.e. a bridge. (Requires sudo)"
sudo $INCLUDEOS_SRC/etc/create_bridge.sh

echo -e "\n\n>>> Done! Test your installation with ./test.sh"
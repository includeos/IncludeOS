#!/bin/bash

# Will install IncludeOS on ubuntu using the install_from_bundle script.sh
# Dependencies and a network bridge used by qemu is also installed. 
#
#
# OPTIONS:
#
# Location of the IncludeOS repo:
# $ export INCLUDEOS_SRC=your/github/cloned/IncludeOS

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}

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
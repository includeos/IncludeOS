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

# Figure out release specific options
UBUNTU_VERSION=`lsb_release -rs`

if [ $(echo "$UBUNTU_VERSION < 16.04" | bc) -eq 1 ]
then
      clang_version=3.6
      EXTRA_DEPENDENCIES="gcc-5 g++-5"
      sudo add-apt-repository ppa:ubuntu-toolchain-r/test
fi

# default_settings
clang_version=${clang_version-3.8}
EXTRA_DEPENDENCIES=${EXTRA_DEPENDENCIES-""} # If no extra dependencies are needed, it stays blank

# Install dependencies

DEPENDENCIES="curl make clang-$clang_version nasm bridge-utils qemu $EXTRA_DEPENDENCIES"
echo ">>> Installing dependencies (requires sudo):"
echo "    Packages: $DEPENDENCIES"
sudo apt-get update
sudo apt-get install -y $DEPENDENCIES

echo -e "\n\n>>> Calling install_from_bundle.sh script"
$INCLUDEOS_SRC/etc/install_from_bundle.sh


echo -e "\n\n>>> Creating a virtual network, i.e. a bridge. (Requires sudo)"
sudo $INCLUDEOS_SRC/etc/create_bridge.sh

echo -e "\n\n>>> Done! Test your installation with ./test.sh"

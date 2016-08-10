#!/bin/bash

# Will install IncludeOS on ubuntu using the install_from_bundle script.sh
# Dependencies and a network bridge used by qemu is also installed. 
#

export INCLUDEOS_SRC=`pwd`
FEDORA_VERSION=`lsb_release -rs`

if [ "24" != "$FEDORA_VERSION" ]
then
    echo "Only Fedora 24 is supported"
    exit 1
fi

DEPENDENCIES="curl make clang nasm bridge-utils qemu"
echo ">>> Installing dependencies (requires sudo):"
echo "    Packages: $DEPENDENCIES"
sudo dnf install $DEPENDENCIES

echo -e "\n\n>>> Calling install_from_bundle.sh script"
./etc/install_from_bundle.sh


echo -e "\n\n>>> Creating a virtual network, i.e. a bridge. (Requires sudo)"
sudo ./etc/create_bridge.sh

echo -e "\n\n>>> Done! Test your installation with ./test.sh"

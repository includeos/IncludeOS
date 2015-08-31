#!/bin/bash

# Enough stuff to install the toolchain
sudo apt-get install -y gcc g++ build-essential qemu-kvm make nasm bridge-utils

echo -e "\n\n>>> BUILDING CROSS COMPILER \n"
./cross_compiler.sh

echo -e "\n\n>>> BUILDING NEWLIB \n"
./build_newlib.sh

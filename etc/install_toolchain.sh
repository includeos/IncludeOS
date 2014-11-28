#! /bin/bash

#Enough stuff to create a basic image and run it
sudo apt-get install -y gcc g++ build-essential emacs qemu-kvm make nasm bridge-utils 

echo -e "\n\n>>> BUILDING CROSS COMPILER \n"
sudo ./etc/cross_compiler.sh

echo -e "\n\n>>> BUILDING NEWLIB \n"
sudo ./etc/build_newlib.sh

#! /bin/bash

INSTALL_DIR=/usr/local/IncludeOS/

export PREFIX=$INSTALL_DIR
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
export build_dir=$HOME/cross-dev

# Multitask-parameter to make
export num_jobs=-j8

export binutils_version=2.25
export gcc_version=5.1.0
export newlib_version=2.2.0-1


# Utilities
. ./etc/bash_functions.sh


# TODO: Implement checks to see which steps can be skipped!


# BUILDING IncludeOS

PREREQS_BUILD="gcc g++ build-essential make nasm texinfo"

# Get prerequisite packages, such as a compiler, GNU Make, etc.
echo -e "\n\n >>> Trying to install prerequisites for *building* IncludeOS"
echo -e  "        Packages: $PREREQS_BUILD \n"
sudo apt-get install -y $PREREQS_BUILD
or_die "Couldn't install required packages"


# Get and build the actual toolchain
echo -e "\n\n >>> GETTING / BUILDING CROSS COMPILER \n"
 ./etc/cross_compiler.sh
or_die "Couldn't install cross compiler"


echo -e "\n\n >>> GETTING / BUILDING NEWLIB \n"
./etc/build_newlib.sh
or_die "Couldn't install newlib"

# Build and install the vmbuilder 
echo -e "\n >>> Installing vmbuilder"
cd vmbuild
make
sudo cp vmbuild $OSDIR/
or_die "Couldn't install vmbuilder"
 
echo -e "\n >>> Building IncludeOS"
cd ../src
make 
or_die "Couldn't build IncludeOS"

echo -e "\n >>> Installing IncludeOS"
sudo make install
cd ..
or_die "Couldn't install IncludeOS"


# RUNNING IncludeOS
PREREQS_RUN="bridge-utils qemu-kvm"
echo -e "\n\n >>> Trying to install prerequisites for *running* IncludeOS"
echo -e   "        Packages: $PREREQS_RUN \n"
sudo apt-get install -y $PREREQS_RUN
or_die "Couldn't install packages needed to run IncludeOS"

# Set up the IncludeOS network bridge
echo -e "\n\n >>> Create IncludeOS network bridge "`pwd`"\n"
sudo ./etc/create_bridge.sh
sudo cp ./etc/qemu-ifup /etc
or_die "Couldn't set up the network bridge"


echo -e "\n >>> Done. Test the installation by running ./test.sh \n"


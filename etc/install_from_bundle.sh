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

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}
INCLUDEOS_INSTALL_LOC=${INCLUDEOS_INSTALL_LOC-$HOME}
INCLUDEOS_INSTALL=${INCLUDEOS_INSTALL-$INCLUDEOS_INSTALL_LOC/IncludeOS_install}

# Find the latest release
echo -e "\n\n>>> Getting the ID of the latest release from GitHub"
JSON=`curl https://api.github.com/repos/hioa-cs/IncludeOS/releases`
FILENAME=`echo "$JSON" | jq -r '.[0]["assets"][0]["name"]'`
DOWNLOAD_URL=`echo "$JSON" | jq -r '.[0]["assets"][0]["browser_download_url"]'`
echo -e "\n\n>>> File to download: $DOWNLOAD_URL"

# If the tarball exists, use that
if [ -e $FILENAME ]
then
    echo -e "\n\n>>> IncludeOS tarball exists - extracting to $INCLUDEOS_INSTALL_LOC"
else
    # Download from GitHub API
    echo -e "\n\n>>> Downloading latest IncludeOS release tarball from GitHub"
    curl -H "Accept: application/octet-stream" -L -o $FILENAME $DOWNLOAD_URL
fi

# Extracting the downloaded tarball
echo -e "\n\n>>> Fetched tarball - extracting to $INCLUDEOS_INSTALL_LOC/IncludeOS_install"
gunzip $FILENAME -c | tar -C $INCLUDEOS_INSTALL_LOC -xf -   # Pipe gunzip to tar

# Install submodules
echo -e "\n\n>>> Installing submodules"
pushd $INCLUDEOS_SRC
git submodule init
git submodule update
popd

# Build IncludeOS
echo -e "\n\n>>> Building IncludeOS"
pushd $INCLUDEOS_SRC/src
make -j
make install
popd

# Compile vmbuilder
echo -e "\n\n>>> Compiling the vmbuilder, which makes a bootable vm out of your service"
pushd $INCLUDEOS_SRC/vmbuild
make
cp vmbuild $INCLUDEOS_INSTALL/
popd

# Copy scripts for running qemu, creating a memdisk
$INCLUDEOS_SRC/etc/copy_scripts.sh

echo -e "\n\n>>> Done! IncludeOS bundle downloaded and installed"

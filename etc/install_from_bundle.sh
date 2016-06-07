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


echo ">>> Updating git-tags "
# Get the latest tag from IncludeOS repo
tag=`git ls-remote --tags https://github.com/hioa-cs/IncludeOS.git | grep -oP "tags/\K(.*)$" | grep -v "{" | sort -n -t . -k 1 -k 2 -k 3 | tail -n 1`

filename_tag=`echo $tag | tr . -`
filename="IncludeOS_install_"$filename_tag".tar.gz"

# If the tarball exists, use that
if [ -e $filename ]
then
    echo -e "\n\n>>> IncludeOS tarball exists - extracting to $INCLUDEOS_INSTALL_LOC"
    tar -C $INCLUDEOS_INSTALL_LOC -xzf $filename
else
    echo -e "\n\n>>> Downloading IncludeOS release tarball from GitHub"
    # Download from GitHub API
    if [ "$1" = "-oauthToken" ]
    then
        oauthToken=$2
        echo -e "\n\n>>> Getting the ID of the latest release from GitHub"
        JSON=`curl -u $git_user:$oauthToken https://api.github.com/repos/hioa-cs/IncludeOS/releases/tags/$tag`
    else
        echo -e "\n\n>>> Getting the ID of the latest release from GitHub"
        JSON=`curl https://api.github.com/repos/hioa-cs/IncludeOS/releases/tags/$tag`
    fi
    ASSET=`echo $JSON | $INCLUDEOS_SRC/etc/get_latest_binary_bundle_asset.py`
    ASSET_URL=https://api.github.com/repos/hioa-cs/IncludeOS/releases/assets/$ASSET

    echo -e "\n\n>>> Getting the latest release bundle from GitHub"
    if [ "$1" = "-oauthToken" ]
    then
        curl -H "Accept: application/octet-stream" -L -o $filename -u $git_user:$oauthToken $ASSET_URL
    else
        curl -H "Accept: application/octet-stream" -L -o $filename $ASSET_URL
    fi

    echo -e "\n\n>>> Fetched tarball - extracting to $INCLUDEOS_INSTALL_LOC"
    tar -C $INCLUDEOS_INSTALL_LOC -xzf $filename
fi

echo -e "\n\n>>> Installing submodules"
pushd $INCLUDEOS_SRC
git submodule init
git submodule update
popd

echo -e "\n\n>>> Building IncludeOS"
pushd $INCLUDEOS_SRC/src
make -j
make install
popd

echo -e "\n\n>>> Compiling the vmbuilder, which makes a bootable vm out of your service"
pushd $INCLUDEOS_SRC/vmbuild
make
cp vmbuild $INCLUDEOS_HOME/
popd

echo -e "\n\n>>> Creating a virtual network, i.e. a bridge. (Requires sudo)"
sudo $INCLUDEOS_SRC/etc/create_bridge.sh

$INCLUDEOS_SRC/etc/copy_scripts.sh

echo -e "\n\n>>> Done! Test your installation with ./test.sh"

#!/bin/bash -x

# Install the IncludeOS libraries (i.e. IncludeOS_home) needed to build an IncludeOS image 
# from binary bundle
#
# OPTIONS: 
#
# Location of the IncludeOS repo:
# $ export INCLUDEOS_SRC=your/github/cloned/IncludeOS
#
# Parent directory of where you want the IncludeOS libraries (i.e. IncludeOS_home)
# $ export INCLUDEOS_INSTALL_LOC=parent/folder/for/IncludeOS/libraries i.e. 
#
# To install a special release, add the release as an option:
# ./install_build_only.sh v0.7.0-proto
#
[ ! -v INCLUDEOS_SRC ] && export INCLUDEOS_SRC=`pwd`
[ ! -v INCLUDEOS_INSTALL_LOC ] && export INCLUDEOS_INSTALL_LOC=$HOME
export INCLUDEOS_HOME=$INCLUDEOS_INSTALL_LOC/IncludeOS_install
mkdir -p $INCLUDEOS_HOME

VERSION=$1

# Install dependencies
DEPENDENCIES="curl make clang-3.6 nasm git"
echo ">>> Installing dependencies (requires sudo):"
echo "    Packages: $DEPENDENCIES"
sudo apt-get update
sudo apt-get install -y $DEPENDENCIES

tag=$VERSION

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
    
    echo -e "\n\n>>> Getting the ID of the latest release from GitHub"
    JSON=`curl https://api.github.com/repos/hioa-cs/IncludeOS/releases/tags/$tag`

    ASSET=`echo $JSON | $INCLUDEOS_SRC/etc/get_latest_binary_bundle_asset.py`
    ASSET_URL=https://api.github.com/repos/hioa-cs/IncludeOS/releases/assets/$ASSET

    curl -H "Accept: application/octet-stream" -L -o $filename  $ASSET_URL
    
    echo -e "\n\n>>> Fetched tarball - extracting to $INCLUDEOS_INSTALL_LOC"
    tar -C $INCLUDEOS_INSTALL_LOC -xzf $filename    
fi

echo -e "\n\n>>> Checking out correct IncludeOS release"
pushd $INCLUDEOS_SRC
git checkout $tag
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





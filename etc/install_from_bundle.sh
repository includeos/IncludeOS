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

# Q: why are you downloading a binary installation of includeos during the
#    install.sh script? Would have thought that having a full git checkout
#    should be enough to build the system..
# A: We're not downloading a binary of IncludeOS - just all the libraries it needs
#    to build os.a (the IncludeOS library), such as libc++ and libc. To build
#    those you need to do the whole --from-source thing which takes one hour,
#    and we don't want people to have to do that. The bundle that gets downloaded
#    actually gets created during install from source - you can see it here: 
#    https://github.com/hioa-cs/IncludeOS/blob/master/etc/create_binary_bundle.sh
#
# Q: hm, the normal libc++ and libc libraries (available through the package
#    management system) are not enough? 
# A: No, the normal libc++ is GNU (actually libstdc++) and we're on clang. Also it
#    has to be 32-bit and the libc we have implements a much smaller set of system
#    calls than you have on Linux. Another snag is that you need to compile libc++
#    with the actual libc you're in fact using, so we need to first build custom
#    libc (newlib), then build libc++ using the header files from that one. In
#    addition we need to build the C and C++ ABI's, which include stack unwinding etc.
#    Another snag is that building newlib ... we've tried and can't do it without GCC.
#    The stack unwinder is also GCC specific (form libgcc) so we need GCC
#    to build those - and in order to build for a custom target we also
#    need to build GCC from source. Hence building from source implies building
#    all of gcc from scratch, binutils, libc and libc++ ... and it takes an hour
#    - or 15-20 mins. on a 48 core machine and $ make -j :-D
#
#    Real horrorshow, but in the end we worked hard to make the IncludeOS bundle
#    be as lean as possible, only containing static libraries and header files
#    for exactly what you need to build IncludeOS, nothing more, nothing less.
#    Surely, it would be great to get this all nicer, but then again, they're
#    just dependency libraries - very much like getting binaries using "apt-get install".

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

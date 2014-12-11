#! /bin/bash
#
# Build VirtualBox from source
# 
# Assumes you have downloaded the virtualbox svn sources into "./vbox"

cd vbox
./configure --disable-hardening
. env.sh 

# You need a debug build to get more than vanilla
# (If you want vanilla, use the packages)
export BUILD_TYPE=debug

# Build it
kmk VBOX_WITH_R0_LOGGING=1

# Now build the kernel modules

export BUILD_TYPE=debug
cd ./vbox/out/linux.amd64/debug/bin/src/
sudo make
sudo make install

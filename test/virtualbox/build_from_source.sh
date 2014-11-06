#! /bin/bash
#
# Build VirtualBox from source
# 
# Assumes you have downloaded the virtualbox svn sources into "./vbox"

cd vbox
#.configure --disable-hardening
. env.sh 

# You need a debug build to get more than vanilla
# (If you want vanilla, use the packages)
export BUILD_TYPE=debug

# Build it
kmk

# Now build the kernel modules


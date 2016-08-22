#!/bin/bash

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS/}
INCLUDEOS_INSTALL=${INCLUDEOS_INSTALL-$HOME/IncludeOS_install}
mkdir -p $INCLUDEOS_INSTALL/etc
cp $INCLUDEOS_SRC/etc/to-be-installed/qemu-ifup $INCLUDEOS_INSTALL/etc/
cp $INCLUDEOS_SRC/etc/to-be-installed/qemu_cmd.sh $INCLUDEOS_INSTALL/etc/
cp $INCLUDEOS_SRC/etc/to-be-installed/run.sh $INCLUDEOS_INSTALL/etc/
cp $INCLUDEOS_SRC/etc/to-be-installed/create_memdisk.sh $INCLUDEOS_INSTALL/etc/

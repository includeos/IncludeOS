#!/bin/bash

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS/}
INSTALL_DIR=${INSTALL_DIR-$HOME/IncludeOS_install}
mkdir -p $INSTALL_DIR/etc
cp $INCLUDEOS_SRC/etc/qemu-ifup $INSTALL_DIR/etc/
cp $INCLUDEOS_SRC/etc/qemu_cmd.sh $INSTALL_DIR/etc/


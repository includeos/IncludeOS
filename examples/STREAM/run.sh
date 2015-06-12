#!/bin/bash
image=$1

# Get the Qemu-command (in-source, so we can use it elsewhere)
. ./qemu_cmd.sh

make clean all #stripped 
echo "-----------------------"
echo "Starting VM: '$image'", "Options: ", $QEMU_OPTS
echo "-----------------------"

$QEMU $QEMU_OPTS

#! /bin/bash

# Start as a GDB service, for debugging
# (0 means no, anything else yes.)

[[ $2 = "debug" ]] && DEBUG=1 || DEBUG = 0

DEV_NET="-net nic,model=virtio"
DEV_GRAPHICS="-nographic"
DEV_HDD="-hda $1"
QEMU_OPTS="$DEV_HDD $DEV_NET $DEV_GRAPHICS"

# Qemu with gdb debugging:

if [ $DEBUG -ne 0 ]
then
    echo "Building system..."
    make -B debug
    echo "Starting VM: '$1'"
    echo "-----------------------"
    
    echo "VM started in DEBUG mode. Connect with gdb/emacs:"
    echo " - M+x gdb, Enter, then start with command"
    echo "   gdb -i=mi service -x service.gdb"
    echo "-----------------------"  
    qemu-system-x86_64 -s -S $QEMU_OPTS
else    
    make clean stripped 
    echo "-----------------------"
    echo "Starting VM: '$1'", "Options: ",$QEMU_OPTS
    echo "-----------------------"
    
    qemu-system-x86_64 $QEMU_OPTS 
fi

#reset
#bochs

#! /bin/bash

# Start as a GDB service, for debugging
# (0 means no, anything else yes.)

if [ $2 = "debug" ]
then
    
    DEBUG=1
else
    DEBUG=0
fi

# Qemu with gdb debugging:

if [ $DEBUG -ne 0 ]
then
    echo "Building system..."
    make -B debug
    ./vmbuilder
    echo "Starting VM: '$1'"
    echo "-----------------------"
    
    echo "VM started in DEBUG mode. Connect with gdb/emacs:"
    echo " - M+x gdb, Enter, then start with command"
    echo "   gdb -i=mi service -x service.gdb"
    echo "-----------------------"  
    qemu-system-i386 -s -S -hda $1 -nographic
else    
    make -B
    ./vmbuilder
    echo "-----------------------"
    echo "Starting VM: '$1'"
    echo "-----------------------"

    qemu-system-i386 -hda $1 -nographic
fi

#reset
#bochs

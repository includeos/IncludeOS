#! /bin/bash

# Start as a GDB service, for debugging
# (0 means no, anything else yes.)

DEBUG=0
STRIPPED=0
JOBS=12
KERNEL=test_ipv6
IMAGE=$KERNEL.img

[[ $1 = "debug" ]] && DEBUG=1 
[[ $1 = "stripped" ]] && STRIPPED=1 


# Get the Qemu-command (in-source, so we can use it elsewhere)
. debug/qemu_cmd.sh


# Qemu with gdb debugging:
if [ "$DEBUG" -ne 0 ]
then
    echo "Building system..."
    make -j$JOBS all $KERNEL
    
    # Build the image 
    ../vmbuild/vmbuild bootloader $KERNEL
    mv $KERNEL $KERNEL.img debug/

    echo "Starting VM: '$1'"
    echo "-----------------------"    
    
    echo "VM started in DEBUG mode. Connect with gdb/emacs:"
    echo " - M+x gdb, Enter, then start with command"
    echo "   gdb -i=mi service -x service.gdb"
    echo "-----------------------"  
    sudo $QEMU -s -S $QEMU_OPTS
    
elif [ "$STRIPPED" -ne 0 ]; then
    
    make -j$JOBS stripped $KERNEL
    mv $KERNEL $KERNEL.img debug/
    
    # Build the image 
    ../vmbuild/vmbuild bootloader $KERNEL

    echo "-----------------------"
    echo "Starting VM: '$1'", "Options: ",$QEMU_OPTS
    echo "-----------------------"
    
    sudo $QEMU $QEMU_OPTS 
    
else
    
    make -j$JOBS all $KERNEL
    
    # Build the image 
    ../vmbuild/vmbuild bootloader $KERNEL
    mv $KERNEL $KERNEL.img debug/
    
    echo "-----------------------"
    echo "Starting VM: '$1'", "Options: ",$QEMU_OPTS
    echo "-----------------------"
    
    sudo $QEMU $QEMU_OPTS 
fi

# Convert the image into VirtualBox / Qemu native formats
# . convert_image.sh

#reset
#bochs

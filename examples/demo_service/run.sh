#! /bin/bash

# Start as a GDB service, for debugging
# (0 means no, anything else yes.)
export IMAGE=$1

[ ! -v INCLUDEOS_HOME ] && INCLUDEOS_HOME=$HOME/IncludeOS_install

DEBUG=0

[[ $2 = "debug" ]] && DEBUG=1 

# Get the Qemu-command (in-source, so we can use it elsewhere)
. $INCLUDEOS_HOME/etc/qemu_cmd.sh

# Qemu with gdb debugging:

if [ $DEBUG -ne 0 ]
then
    echo "Building system..."
    make -B debug
    echo "Starting VM: '$1'"
    echo "Command: $QEMU $QEMU_OPTS"
    echo "------------------------------------------------------------"    
    echo "VM started in DEBUG mode. Connect with gdb/emacs:"
    echo " - M+x gdb, Enter, then start with command"
    echo "   gdb -i=mi service -x service.gdb"
    echo "------------------------------------------------------------"    

    sudo $QEMU -s -S $QEMU_OPTS
else    

    echo "------------------------------------------------------------"    
    echo "Starting VM: '$1'"
    echo "------------------------------------------------------------"        
    sudo $QEMU $QEMU_OPTS 
fi

echo "NOTE: To run you image on another platform such as virtualbox, check out IncludeOS/etc/convert_image.sh"

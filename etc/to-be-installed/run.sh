#! /bin/bash

if [[ $# -lt 1 ]]; then
    echo "Usage: IMAGE.img [debug]"
    exit $1
fi

export IMAGE=$1

INCLUDEOS_HOME=${INCLUDEOS_HOME-$HOME/IncludeOS_install}

# Start as a GDB service, for debugging
# (0 means no, anything else yes.)
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

    echo $QEMU -s -S $QEMU_OPTS $QEMU_EXTRA
    sudo $QEMU -s -S $QEMU_OPTS $QEMU_EXTRA
else    

    echo "------------------------------------------------------------"    
    echo "Starting VM: '$1'"
    echo "------------------------------------------------------------"        
    echo $QEMU $QEMU_OPTS $QEMU_EXTRA
    sudo $QEMU $QEMU_OPTS $QEMU_EXTRA
fi

echo "NOTE: To run your image in another VMM, such as VirtualBox, check out IncludeOS/etc/convert_image.sh"

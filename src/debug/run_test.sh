#!/bin/bash
IMAGE=debug/$SERVICE.img

# Start as a GDB service, for debugging
# (0 means no, anything else yes.)
DEBUG=0
STRIPPED=0
[[ $1 = "debug" ]] && DEBUG=1
[[ $1 = "stripped" ]] && STRIPPED=1
[[ $2 = "stripped" ]] && STRIPPED=1

echo "Building system $SERVICE..."

# Get the Qemu-command (in-source, so we can use it elsewhere)
export CPU="-cpu host,+kvm_pv_eoi "
. ../etc/qemu_cmd.sh
export SERIAL="" #"-monitor none -virtioconsole stdio"
QEMU_OPTS+=" -drive file=./smalldisk,if=virtio,media=disk $SERIAL"

# Qemu with gdb debugging:
if [ "$DEBUG" -ne 0 ]
then
    make -j$JOBS debug $SERVICE
    
    # Build the image 
    ../vmbuild/vmbuild bootloader debug/$SERVICE
    mv $SERVICE.img debug/

    echo "Starting VM: '$IMAGE'"
    echo "---------------------------------------------------------------------------------"
    echo "VM started in DEBUG mode. Connect with gdb/emacs:"
    echo " - M+x gdb, Enter, then start with command"
    echo "   gdb -i=mi service -x service.gdb"
    echo "---------------------------------------------------------------------------------"
    
    QEMU="qemu-system-i386"
    sudo $QEMU -s -S $QEMU_OPTS
    
elif [ "$STRIPPED" -ne 0 ]
then
    make -j$JOBS stripped $SERVICE
    
    echo ">>> Stripping $SERVICE"
    strip --strip-all -R.comment debug/$SERVICE
    
    # Build the image 
    ../vmbuild/vmbuild bootloader debug/$SERVICE
    mv $SERVICE.img debug/
    echo "---------------------------------------------------------------------------------"
    echo "Starting VM: '$IMAGE'", "Options: ",$QEMU_OPTS
    echo "---------------------------------------------------------------------------------"
    sudo $QEMU $QEMU_OPTS 
else
    make -j$JOBS all 
    make $SERVICE
    
    # Build the image 
    ../vmbuild/vmbuild bootloader debug/$SERVICE
    mv $SERVICE.img debug/
    echo "---------------------------------------------------------------------------------"
    echo "Starting VM: '$IMAGE'", "Options: ",$QEMU_OPTS
    echo "---------------------------------------------------------------------------------"
    sudo $QEMU $QEMU_OPTS
fi

# Convert the image into VirtualBox / Qemu native formats
# . convert_image.sh

#reset
#bochs

#! /bin/bash

#
# Run and re-run an Include OS vm after compilation, properly replacing the disk.
#
# MOTIVATION: 
# When you replace a virtual box disk image with a newer version (such as one 
# newly created by ./test.sh) any VM using this image file will fail. For this 
# reason you could replace the image file manually, but then virtualbox
# will complain about the UUID not matching the UUID in the image registry. 
# This script properly replaces the disk attached to a VM with a new one.
# 
# PREREQUISITES:
# 1. Create a new VirtualBox VM, called whatever you set in $VM_NAME
# 2. Add an existing disk, and point to the image created by "IncludeOS/test.sh"
#    usually IncludeOS/seed/IncludeOS_tests.img.vdi
# 4. Set up a serial port for the VM under "Settings->Ports" and:
#    - Check Enable serial port
#    - Com1, defaults
#    - Port mode: Raw file
#    - Port/File path: whatever you put in $SERIAL_FILE below
# 3. Run the VM and make sure it works
# 4. After this you should be able to use this script
#
# USAGE: 
# 1. Compile / build an image, for instance with $ <IncludeOS>/test.sh
# (Now make the script copy <IncludeOS>/seed/IncludeOS_test.img.vdi to wherever
# you're running this script.)
# 
# 2. $ ./vboxrun.sh
# (now the VM should start, and the serial port output be displayed)
# C^ c
# (now the VM should stop)
# 
# 3. Make your changes, and repeat 1.


export VM_NAME="IncludeOS_test1"
export DISK_RAW="./IncludeOS_tests.img"
export DISK="./IncludeOS_tests.img.vdi"

if [ "$1" != "" ]
then
    export DISK=$1
fi

export UUID="4c29f994-ce59-4ddf-ba6b-46bcff01c321"
export SERIAL_FILE="./virtualbox_serial.out"

#"981C60DF-F54C-4244-868E-52EA87AC1E8A"
export vbox="./VBoxManage_debug"

# Clear logs
rm *.log

# Convert the image, keeping the UUID intact
#$vbox convertfromraw $DISK_RAW $DISK --uuid $UUID

# Detach the IDE disk
$vbox storageattach $VM_NAME --storagectl "IDE" --port 0 --device 0 \
    --medium "none"

# Remove from media registry
$vbox closemedium disk $DISK 

# Reattach 
$vbox storageattach $VM_NAME --storagectl "IDE" --port 0 --device 0 \
    --medium $DISK --type "hdd" --setuuid $UUID

sleep 0.5

# Start the VM
$vbox startvm $VM_NAME

# Watch the serial port log (requires setting up a serial to file)
watch tail $SERIAL_FILE

# Power off VM
$vbox controlvm $VM_NAME poweroff 

cat *.log

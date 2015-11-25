#!/bin/bash

# MOTIVATION:
# When you replace a virtual box disk image with a newer version (such as one
# newly created by ./test.sh) any VM using this image will fail. For this
# reason you could replace the image file manually, but then virtualbox will
# complain about UUID not matching the UUID in the image registry.
# This script properly replaces the disk attached to a VM with a new one.

# PREREQUISITES
# 1. Create a new VirtualBox VM, called whatever you set in $vmName
# 2. Add an existing disk, and point to the image created by "IncludeOS/test.sh"
# usually IncludeOS/seed/IncludeOS_tests.img.vdi
# 3. Set up a serial port for the VM under "Settings->Ports" and:
#    - Check Enable serial port
#    - Com1, defaults
#    - Port mode: Raw file
#    - Port/File path: Whatever you put in $SERIAL_FILE below
# 4. Run the VM and make sure it works
# 5. After this you should be able to use this script

# USAGE:
# 1. Compile/build an image, for instance with $<IncludeOS>/test.sh
# (Now make the script copy <IncludeOS/seed/IncludeOS_test.img.vdi to
# wherever you're running this script)
#
# 2. $ ./vboxrun.sh VIRTUAL_DRIVE_PATH NAME_OF_THE_VM
# (After the script has run the VM should start,
# and the serial port output be displayed)
# C^c
# (Now the VM should stop)
#
# 3. Make your changes, and repeat 1.

VB=VBoxManage
VMNAME="IncludeOS_test"
homeDir=$(eval echo ~${SUDO_USER})

disk="$homeDir/IncludeOS/seed/My_IncludeOS_Service.img"
targetLoc="$homeDir/IncludeOS/seed/My_IncludeOS_Service.vdi"

# Checks if the disk path parameter $1 is set if not,
# use the default image located in /seed
[ "$1" != "" ] && disk=$1

# Checks if the VM name parameter $2 is set if not,
# replace the default VM created by this script
[ "$2" != "" ] && VMNAME=$2

# Checks if the disk in question is of the .vdi format.
# if not, we convert it and save it in current dir.
# If the disk is in use by the VM that is selected in $2
# then we skip and handle the disk exchange later.
# Else we clone the disk and use it
DISKUSED="$(VBoxManage list hdds -l | grep "Location:" | awk '{print $2}')"
DISKUSEDBY="$(VBoxManage list hdds -l | grep "In use by VMs" | awk '{print $5}')"

if [ "$disk" != *".vdi"* ]
then
	targetLoc=${disk%.*}
	targetLoc=${targetLoc##*/}
	targetLoc+=".vdi"
	displayLoc=$targetLoc
	targetLoc="$(pwd)/$targetLoc"
	echo -e "\nCreating VDI image in $(pwd)/$displayLoc...\n"
	$VB convertfromraw $disk $targetLoc
elif [ "$DISKUSED" == "$(pwd)/$disk" ] && [ "$DISKUSEDBY" != "$VMNAME" ]
then
	targetLoc=${DISKUSED##*/}
	targetLoc="$(pwd)/$VMNAME-$targetLoc"
	$VB clonehd "$DISKUSED" "$targetLoc" --format VDI
else
	targetLoc="$disk"
fi

SERIAL_FILE="./$VMNAME.console.pipe"
if [ -f "./$VMNAME.console.pipe" ]
then
	mv "./$VMNAME.console.pipe" "./$VMNAME.console.pipe.old"
fi

# Checks if the virtualdisk image is in use by another VM
# if it is, detach it from the VM and replace UUID


# Check if VM exists, if yes remove disk and re-attach with a legitimate UUID
# asks user if the VM should be replaced, if not then exits. Run again with
# different NAME_OF_VM
# TODO: MAKE THIS LOGIC WORK & ADD LOGIC TO ALLOW USER TO REPLACE VM NAME ON THE FLY
vmAlive=$($VB list vms | grep "$VMNAME")

if [ "$vmAlive" != "" ]
then
	echo -e "\nVM already exists or in use, replacing it...\n"
# Not sure if needed as the script powers off the VM by default when it finishes
#	$VB controlvm "$VMNAME" poweroff

# We need to remove the disk and reattach it with a different UUID to calm VirtualBox
	$VB modifyvm "$VMNAME" --hda none
	$VB closemedium disk "$targetLoc"
	$VB internalcommands sethduuid "$targetLoc"
	$VB modifyvm "$VMNAME" --hda "$targetLoc"
else
	$(rm $homeDir/VirtualBox\ VMs/$VMNAME/$VMNAME.vbox)
# Creating and registering the VM and adding a virtual IDE drive to it,
# then attaching the hdd image.
	echo -e "\nCreating VM: $VMNAME ...\n"
	$VB createvm --name "$VMNAME" --ostype "Other" --register
	$VB storagectl "$VMNAME" --name "IDE Controller" --add ide --bootable on
	$VB storageattach "$VMNAME" --storagectl "IDE Controller" --port 0 --device 0 --type 'hdd' --medium "$targetLoc"

# Set the boot disk
	$VB modifyvm "$VMNAME" --boot1 disk

# Serial port configuration to receive output
	$VB modifyvm "$VMNAME" --uart1 0x3F8 4 --uartmode1 file $SERIAL_FILE

# NETWORK
	$VB modifyvm "$VMNAME" --nic1 hostonly --nictype1 virtio --hostonlyadapter1 include0
fi
# START VM
$VB startvm "$VMNAME" --type headless &
echo -e "\nVM $VMNAME started, processID is $!\n"

echo "--------------------------------------------"
echo "Checking serial output file, CTRL+C to exit."
echo "--------------------------------------------"

# Checks the serialfile produced by the VM
watch tail $SERIAL_FILE

# Shut down the VM
echo "Shutting down VM: $VMNAME..."
$VB controlvm "$VMNAME" poweroff
echo "Goodbye!"

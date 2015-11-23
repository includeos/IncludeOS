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


# Creates a trap for CTRL+C so that the VM
# powers off when the watch command gets interrupted

function control_c
{
        # Shut down the VM
        echo "Shutting down VM: $vmName..."
        $VB controlvm $VMNAME poweroff
	echo "Goodbye!"
}

trap control_c INT

VB=VBoxManage
VMNAME="IncludeOS_test"
SERIAL_FILE="/tmp/IncludeOS.console.pipe"

homeDir=$(eval echo ~${SUDO_USER})

disk="$homeDir/IncludeOS/seed/My_IncludeOS_Service.img"
targetLoc="$homeDir/IncludeOS/seed/My_IncludeOS_Service.vdi"

# Checks if the VM name parameter $2 is set if not,
# replace the default VM created by this script
if [ "$2" != "" ]
then
	VMNAME=$2
fi

# Check if VM exists, if yes remove disk and re-attach with a legitimate UUID
# asks user if the VM should be replaced, if not then exits. Run again with
# different NAME_OF_VM
# TODO: MAKE THIS LOGIC WORK & ADD LOGIC TO ALLOW USER TO REPLACE VM NAME ON THE FLY
if [ "$VB list vms | grep $VMNAME" ]
then
	echo -e "\nVM already exists, replacing it...\n"
	$VB controlvm $VMNAME poweroff
#	echo -e "\nVM already exists, do you want to replace it?\n"
#	while read -n1 -p "[y,n]" vmReplace && [[ "$vmReplace" != "y" ]] || [[  "$vmReplace" != "n" ]]
#	do
#		if [ "$vmReplace" == "y" ] || [ "$vmReplace" == "n" ]
#		then
#			break
#		fi
#		echo -e "\nUnexpected input, please try again\n"
#	done
#
#	if [ "$vmReplace" == "y" ]
#	then
#		echo -e "\nREPLACING VM: $VMNAME with current setup...\n"
#		$VB controlvm $VMNAME poweroff
# Take a virtual disk as an input argument
# if none is given, use the default. Then
# checks if the drive exists in .vdi format,
# if not we convert it to one
	if [ "$1" != "" ]
	then
		echo -e "\nCREATING VM: $VMNAME WITH A CUSTOM VIRTUAL DISK...\n"
		disk=$1
		if [ "$disk" != *".vdi"* ]
		then
			targetLoc=${disk%.*}
			targetLoc=${targetLoc##*/}
			targetLoc+=".vdi"
			targetLoc="./$targetLoc"
			displayLoc=${targetLoc##*/}
			echo -e "\nTEST1 Creating VDI image in $(pwd)/$displayLoc...\n"
			$VB convertfromraw $disk $targetLoc
		fi
	else
		echo -e "\nCREATING VM: $VMNAME WITH THE DEFAULT CONFIGURATION...\n"
		if [ ! -f "$targetLoc" ]
		then
			targetLoc=$disk
			targetLoc=${disk%.*}
#			targetLoc=${disk##*/}
			targetLoc+=".vdi"
#			targetLoc="./$targetLoc"
			echo -e "\nTEST 2Creating VDI image in $targetLoc...\n"
			$VB convertfromraw $disk $targetLoc
		fi
	fi
#	elif [ "$vmReplace" == "n" ]
#	then
#		echo -e "\nNOT replacing $VMNAME please select a different VM name, exiting...\n"
#		exit 0
#	fi
# We need to remove the disk and reattach it with a different UUID to calm VirtualBox
	$VB modifyvm $VMNAME --hda none
	$VB closemedium disk $targetLoc
	$VB internalcommands sethduuid $targetLoc
	$VB modifyvm $VMNAME --hda $targetLoc
else
# Take a virtual disk as an input argument
# if none is given, use the default. Then
# checks if the drive exists in .vdi format,
# if not we convert it to one
	if [ "$1" != "" ]
	then
		echo -e "\nCREATING VM: $VMNAME WITH A CUSTOM VIRTUAL DISK...\n"
        	disk=$1
	        if [ "$disk" != *".vdi"* ]
        	then
                	targetLoc=${disk%.*}
			targetLoc=${targetLoc##*/}
        	        targetLoc+=".vdi"
	                targetLoc="./$targetLoc"
			displayLoc=${targetLoc#*/}
                	echo -e "\nTEST 3Creating VDI image in $(pwd)/$displayLoc...\n"
	                $VB convertfromraw $disk $targetLoc
        	fi
	else
		echo -e "\nCREATING VM: $VMNAME WITH THE DEFAULT CONFIGURATION...\n"
        	if [ ! -f "$targetLoc" ]
	        then
			targetLoc=$disk
        	        targetLoc=${disk%.*}
#        	       	targetLoc=${disk##*/}
	                targetLoc+=".vdi"
#	       	        targetLoc="./$targetLoc"
	                echo -e "\nTEST 4Creating VDI image in $targetLoc...\n"
        	        $VB convertfromraw $disk $targetLoc
	        fi
	fi

# Creating and registering the VM and adding a virtual IDE drive to it,
# then attaching the hdd image.
	echo -e "\nCreating VM: $VMNAME ...\n"
	$VB createvm --name $VMNAME --ostype 'Other' --register
	$VB storagectl $VMNAME --name 'IDE Controller' --add ide --bootable on
	$VB storageattach $VMNAME --storagectl 'IDE Controller' --port 0 --device 0 --type 'hdd' --medium $targetLoc

# Some management
	$VB modifyvm $VMNAME --boot1 disk

# Serial port configuration to receive output
	$VB modifyvm $VMNAME --uart1 0x3F8 4 --uartmode1 file $SERIAL_FILE

# NETWORK
	$VB modifyvm $VMNAME --nic1 hostonly --nictype1 virtio --hostonlyadapter1 include0
	$VB modifyvm $VMNAME --macaddress1 c001A0A0A0A0
fi
# START VM
$VB startvm $VMNAME --type headless &
echo -e "\nVM $VMNAME started, processID is $!\n"

echo "--------------------------------------------"
echo "Checking serial output file, CTRL+C to exit."
echo "--------------------------------------------"

# Checks the serialfile produced by the VM
watch tail $SERIAL_FILE

#!/bin/bash

# Creates a trap for CTRL+C so that the VM
# powers off when the watch command gets interrupted
function control_c
{
        # Shut down the VM
        echo "Shutting down VM: $vmName..."
        $VB controlvm $vmName poweroff
	echo "Goodbye!"
}

trap control_c INT

VB=VBoxManage
vmName=IncludeOS_test

homeDir=$(eval echo ~${SUDO_USER})

disk="$homeDir/IncludeOS/seed/My_IncludeOS_Service.img"
targetLoc="$homeDir/IncludeOS/seed/My_IncludeOS_Service.vdi"

echo -e "\nCreating virtual harddrive...\n"
# CONVERT IMAGE TO VDI HERE
if [ $(ls $homeDir/IncludeOS/seed/ | grep .vdi) ];
then
	echo -e "\nVDI image aldready exists, moving on...\n"
else
	$VB convertfromraw $disk $targetLoc
	echo -e "\nCreating VDI image...\n"
fi

#echo $disk
#echo $targetLoc


# Creating and registering the VM and adding a virtual IDE drive to it,
# then attaching the hdd image.
echo -e "\nCreating VM ...\n"
$VB createvm --name IncludeOS_test --ostype 'Other' --register
$VB storagectl $vmName --name 'IDE Controller' --add ide --bootable on
$VB storageattach $vmName --storagectl 'IDE Controller' --port 0 --device 0 --type 'hdd' --medium $targetLoc


# Some management
$VB modifyvm $vmName --ioapic on
$VB modifyvm $vmName --boot1 disk

# Serial port configuration to receive output
$VB modifyvm $vmName --uart1 0x3F8 4 --uartmode1 file /tmp/IncludeOS.console.pipe

# NETWORK
#$VB hostonlyif create
$VB modifyvm $vmName --nic1 hostonly --nictype1 virtio --hostonlyadapter1 include0
$VB modifyvm $vmName --macaddress1 c001A0A0A0A0

# START VM
$VB startvm $vmName --type headless &
echo -e "\nVM $VmName started, processID is $!\n"

echo "--------------------------------------------"
echo "Checking serial output file, CTRL+C to exit."
echo "--------------------------------------------"

# Checks the serialfile produced by the VM
watch tail /tmp/IncludeOS.console.pipe

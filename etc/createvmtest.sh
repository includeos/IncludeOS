 #!/bin/bash

VB=VBoxManage
vmName=IncludeOS_test

homeDir=$(eval echo ~${SUDO_USER})

disk="$homeDir/IncludeOS/seed/My_IncludeOS_Service.img"
targetLoc="$homeDir/IncludeOS/seed/My_IncludeOS_Service.vdi"

echo "Creating virtual harddrive ..."
# CONVERT IMAGE TO VDI HERE
if [ $(ls $homeDir/IncludeOS/seed/ | grep .vdi) ];
then
	echo "VDI image aldready exists, moving on..."
else
	$VB convertfromraw $disk $targetLoc
	echo "Creating VDI image..."
fi

#echo $disk
#echo $targetLoc

echo 'Creating VM ...'
$VB createvm --name IncludeOS_test --ostype 'Other' --register
$VB storagectl $vmName --name 'IDE Controller' --add ide --bootable on
$VB storageattach $vmName --storagectl 'IDE Controller' --port 0 --device 0 --type 'hdd' --medium $targetLoc


# Some management
$VB modifyvm $vmName --ioapic on
$VB modifyvm $vmName --boot1 disk --boot2 dvd --boot3 none --boot4 none
$VB modifyvm $vmName --memory 1024 --vram 128

# Serial port
$VB modifyvm $vmName --uart1 0x3F8 4 --uartmode1 file /tmp/IncludeOS.console.pipe

# NETWORK
$VB hostonlyif create
$VB modifyvm $vmName --nic1 hostonly --nictype1 virtio --hostonlyadapter1 include0
$VB modifyvm $vmName --macaddress1 c001A0A0A0A0

# START VM
$VB startvm $vmName --type headless &
echo "\nVM $VmName started, processID is $!"

watch tail /tmp/IncludeOS.consle.pipe

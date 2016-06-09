#!/bin/bash
#
# Create, run and rerun IncludeOS VirtualBox VM's 
#
# USAGE:
# 1. Compile/build an image, for instance with $<IncludeOS>/test.sh#
#
# 2. $ ./vboxrun.sh VIRTUAL_DRIVE_PATH NAME_OF_THE_VM
# (After the script has run the VM should start,
# and the serial port output be displayed)
#
# 3. Ctrl-c to stop the VM
# 4. Make your changes to IncludeOS / your service, and repeat 1.


# Set traps (we don't want to continue booting if something fails)
set -e # Exit immediately on error (we're trapping the exit signal)
trap 'previous_command=$this_command; this_command=$BASH_COMMAND' DEBUG
trap 'echo -e "SCRIPT FAILED ON COMMAND: $previous_command\n"' EXIT

VB=VBoxManage
VMNAME="IncludeOS_test"
homeDir=$(eval echo ~${SUDO_USER})

disk="$homeDir/IncludeOS/seed/My_IncludeOS_Service.img"
targetImg="$homeDir/IncludeOS/seed/My_IncludeOS_Service.vdi"

# Checks if the disk path parameter $1 is set if not,
# use the default image located in /seed
[ "$1" != "" ] && disk=$1 && targetImg=$disk

# Checks if the VM name parameter $2 is set if not,
# replace the default VM created by this script
[ "$2" != "" ] && VMNAME=$2 

diskname="${disk%.*}"
targetLoc=$diskname.vdi

# Exit script if disk can not be found.
if [[ ! -e $disk ]]
then
    echo -e "\n>>> '$disk' does not exist. Exiting... "
    exit 1
fi

# Remove disk if already exists, but not if its equal input
if [[ -e $targetLoc && $disk != $targetLoc ]]
then
    echo -e "\n>>> Disk already exists, removing...\n"
    rm $targetLoc
fi

# Convert the disk to .vdi.
# If input == output, no conversion is needed.
if [[ $disk != $targetLoc ]]
then
    $VB convertfromraw $disk $targetLoc
fi

SERIAL_FILE="$(pwd)/$VMNAME.console.pipe"

echo -e "\n>>> Serial port output will be routed to this pipe: $SERIAL_FILE"
[ ! -e $SERIAL_FILE ] && mkfifo $SERIAL_FILE

# Check if VM exists, if yes remove disk and re-attach with a legitimate UUID
# asks user if the VM should be replaced, if not then exits. Run again with
# different NAME_OF_VM

if [ "$($VB list vms | grep "\"$VMNAME\"")" != "" ]
then
    echo -e "\n>>> VM already exists or in use, replacing it...\n"
    
    # We need to remove the disk and reattach it with a different UUID to calm VirtualBox
    $VB modifyvm "$VMNAME" --hda none
    $VB closemedium disk "$targetLoc"
    $VB internalcommands sethduuid "$targetLoc"
    $VB modifyvm "$VMNAME" --hda "$targetLoc"
else
    # Creating and registering the VM and adding a virtual IDE drive to it,
    # then attaching the hdd image.
    echo -e "\nCreating VM: $VMNAME ...\n"
    $VB createvm --name "$VMNAME" --ostype "Other" --register
    $VB storagectl "$VMNAME" --name "IDE Controller" --add ide --bootable on
    $VB storageattach "$VMNAME" --storagectl "IDE Controller" --port 0 --device 0 --type 'hdd' --medium "$targetLoc"

    # Enable I/O APIC
    $VB modifyvm "$VMNAME" --ioapic on
    
    # Set the boot disk
    $VB modifyvm "$VMNAME" --boot1 disk
    
    # Serial port configuration to receive output
    $VB modifyvm "$VMNAME" --uart1 0x3F8 4 --uartmode1 file $SERIAL_FILE
    
    # NETWORK
    $VB modifyvm "$VMNAME" --nic1 hostonly --nictype1 virtio --hostonlyadapter1 vboxnet0

    # Memory
    $VB modifyvm "$VMNAME" --memory 256
    
fi
# START VM
$VB startvm "$VMNAME" &


echo -e "\nVM $VMNAME started, processID is $!\n"

echo "--------------------------------------------"
echo "Checking serial output pipe, CTRL+C to exit."
echo "--------------------------------------------"

# Set new traps for catching exit from "cat"
set +e
trap '$VB controlvm "$VMNAME" poweroff \n' EXIT

# Checks the serialfile produced by the VM
cat $SERIAL_FILE

trap - EXIT

# Check for hardware-virtualization support
if [ "$(egrep -m 1 '^flags.*(vmx|svm)' /proc/cpuinfo)" ]
then
    echo ">>> KVM: ON "
    export QEMU="qemu-system-x86_64 --enable-kvm"
else
    echo ">>> KVM: OFF "
    export QEMU="qemu-system-i386"
fi

export macaddress="c0:01:0a:00:00:2a"
INCLUDEOS_HOME=${INCLUDEOS_HOME-$HOME/IncludeOS_install}
export qemu_ifup="$INCLUDEOS_HOME/etc/qemu-ifup"

export DEV_NET="-device virtio-net,netdev=net0,mac=$macaddress -netdev tap,id=net0,script=$qemu_ifup"
export SMP="-smp 1"

export DEV_GRAPHICS="--nographic" #For a VGA console (which won't work over ssh), use "-vga std"

export SERIAL="-virtioconsole stdio"

export DEV_HDD="-hda $IMAGE"
export QEMU_OPTS="$DEV_HDD $DEV_NET $DEV_GRAPHICS $SMP"

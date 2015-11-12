# Check for hardware-virtualization support
if [ `egrep '^flags.*(vmx|svm)' /proc/cpuinfo` ]
then
    echo ">>> KVM: ON "
    export QEMU="qemu-system-x86_64 --enable-kvm"
else
    echo ">>> KVM: OFF "
    export QEMU="qemu-system-i386"
fi

export macaddress="c0:01:0a:00:00:2a"
[ ! -v INCLUDEOS_INSTALL ] && INCLUDEOS_INSTALL=$HOME/IncludeOS_install
export qemu_ifup="$INCLUDEOS_INSTALL/etc/qemu-ifup"

export DEV_NET="-device virtio-net,netdev=net0,mac=$macaddress -netdev tap,id=net0,script=$qemu_ifup"
export SMP="-smp 1"

export DEV_GRAPHICS="--nographic" #"-vga std"

export DEV_HDD="-hda $1"
export QEMU_OPTS="$DEV_HDD $DEV_NET $DEV_GRAPHICS $SMP"


echo $QEMU $QEMU_OPTS

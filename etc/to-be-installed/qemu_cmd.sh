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

[ ! -v NET ] && export NET="-device virtio-net,netdev=net0,mac=$macaddress -netdev tap,id=net0,script=$qemu_ifup"
[ ! -v SMP ] && export SMP="-smp 4"
[ ! -v GRAPHICS ] && export GRAPHICS="-nographic"
[ ! -v SERIAL ] && export SERIAL="-virtioconsole stdio"
[ ! -v MEM ] && export MEM="-m 128"
[ ! -v HDA ] && export HDA="-drive file=$IMAGE,format=raw,if=ide"
[ ! -v HDB ] && export HDB=""
[ ! -v HDD ] && export HDD="$HDA $HDB"
[ ! -v CPU ] && export CPU=""
[ ! -v KEY ] && export KEY="-k en-us"

export QEMU_OPTS="$HDD $NET $GRAPHICS $SMP $MEM $CPU $KEY"

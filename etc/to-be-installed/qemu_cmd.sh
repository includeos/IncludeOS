# Check for hardware-virtualization support
if [ "$(egrep -m 1 '^flags.*(vmx|svm)' /proc/cpuinfo)" ]
then
    echo ">>> KVM: ON "
    export QEMU="qemu-system-x86_64 --enable-kvm"
    export CPU="-cpu host"
else
    echo ">>> KVM: OFF "
    export QEMU="qemu-system-i386"
    export CPU=""
fi

export macaddress="c0:01:0a:00:00:2a"
INCLUDEOS_HOME=${INCLUDEOS_HOME-$HOME/IncludeOS_install}

export qemu_ifup="$INCLUDEOS_HOME/etc/qemu-ifup"

[ ! -v NET ] && export NET="-device virtio-net,netdev=net0,mac=$macaddress -netdev tap,id=net0,script=$qemu_ifup"
[ ! -v SMP ] && export SMP="-smp 1"
[ ! -v GRAPHICS ] && export GRAPHICS="-nographic"
[ ! -v SERIAL ] && export SERIAL="-virtioconsole stdio"
[ ! -v MEM ] && export MEM="-m 128"
[ ! -v HDA ] && export HDA="-kernel $IMAGE $CMDLINE"
[ ! -v HDB ] && export HDB=""
[ ! -v HDD ] && export HDD="$HDA $HDB"
[ ! -v KEY ] && export KEY="-k en-us"

export QEMU_OPTS="$CPU $HDD $NET $GRAPHICS $SMP $MEM $KEY"

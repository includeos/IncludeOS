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

export NET=${NET-"-device virtio-net,netdev=net0,mac=$macaddress -netdev tap,id=net0,script=$qemu_ifup"}
export SMP=${SMP-"-smp 1"}
export GRAPHICS=${GRAPHICS-"-nographic"}
export SERIAL=${SERIAL-"-virtioconsole stdio"}
export MEM=${MEM-"-m 128"}
export HDA=${HDA-"-kernel $IMAGE $CMDLINE"}
export HDB=${HDB-""}
export HDD=${HDD-"$HDA $HDB"}
export KEY=${KEY-"-k en-us"}

export QEMU_OPTS="$CPU $HDD $NET $GRAPHICS $SMP $MEM $KEY"

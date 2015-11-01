
# Check for hardware-virtualization support
if [ `egrep '^flags.*(vmx|svm)' /proc/cpuinfo` ]
then
    echo ">>> KVM: ON "
    export QEMU="qemu-system-x86_64 --enable-kvm"
else
    echo ">>> KVM: OFF "
    export QEMU="qemu-system-i386"
fi

#DEV_NET="-net nic,model=virtio,macaddr=fa:16:3e:db:40:3a"
#OPENSTACK="-netdev tap,fd=29,id=hostnet0 " #Bad file descriptor
#NETDEV="" #"-netdev tap,id=hostnet0"

#NETDEV="-netdev user,id=hostnet0,net=192.168.76.0/24"
#DEV_NET=$NETDEV "-device virtio-net,netdev=hostnet0,id=net0,mac=fa:16:3e:db:40:3a,bus=pci.0,addr=0x3"

#export DEV_NET="-net nic,model=virtio,macaddr=fa:16:3e:db:40:3a -net user"

# WORKING "user" Networking
#export DEV_NET="-netdev user,id=user.0,hostfwd=tcp::5555-:22 -device virtio-net,netdev=user.0"

#export macaddress="08:00:27:9d:86:e8"
export macaddress="c0:01:0a:00:00:2a"
export DEV_NET="-device virtio-net,netdev=net0,mac=$macaddress -netdev tap,id=net0"
export SMP="-smp 1"
#export DEV_GRAPHICS="-vga std"
export DEV_GRAPHICS="--nographic"

export DEV_HDD="-hda $IMAGE"
export QEMU_OPTS="$DEV_HDD $DEV_NET $DEV_GRAPHICS $SMP"


echo $QEMU $QEMU_OPTS

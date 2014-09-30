
export QEMU="qemu-system-x86_64" # No sudo for "qemy-system-x" # sudo kvm

#DEV_NET="-net nic,model=virtio,macaddr=fa:16:3e:db:40:3a"
#OPENSTACK="-netdev tap,fd=29,id=hostnet0 " #Bad file descriptor
#NETDEV="" #"-netdev tap,id=hostnet0"

#NETDEV="-netdev user,id=hostnet0,net=192.168.76.0/24"
#DEV_NET=$NETDEV "-device virtio-net,netdev=hostnet0,id=net0,mac=fa:16:3e:db:40:3a,bus=pci.0,addr=0x3"

export DEV_NET="-net nic,model=virtio,macaddr=fa:16:3e:db:40:3a -net user"

export DEV_GRAPHICS="-nographic"
export DEV_HDD="-hda $1"
export QEMU_OPTS="$DEV_HDD $DEV_NET $DEV_GRAPHICS"


echo $QEMU $QEMU_OPTS

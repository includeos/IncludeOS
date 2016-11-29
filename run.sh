#! /bin/bash
set -e
IMAGE=$1

$INCLUDEOS_PREFIX/includeos/scripts/create_bridge.sh
sudo qemu-system-x86_64 --enable-kvm --cpu host -kernel $IMAGE -m 128 -nographic -netdev  tap,id=net0,script=$INCLUDEOS_PREFIX/includeos/scripts/qemu-ifup -device virtio-net,netdev=net0,mac=c0:01:0a:00:00:2a

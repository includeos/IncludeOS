#! /bin/bash
set -e
IMAGE=LiveUpdate

../IncludeOS/etc/create_bridge.sh
sudo qemu-system-x86_64 --enable-kvm --cpu host -kernel $IMAGE -m 128 -nographic -netdev  tap,id=net0,script=../IncludeOS/etc/to-be-installed/qemu-ifup -device virtio-net,netdev=net0,mac=c0:01:0a:00:00:2a

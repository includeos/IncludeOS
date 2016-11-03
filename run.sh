#! /bin/bash
set -e
sudo ../IncludeOS/etc/create_bridge.sh
sudo qemu-system-x86_64 --enable-kvm --cpu host -kernel IRCd -m 64 -nographic -netdev  tap,id=net0,script=../IncludeOS/etc/to-be-installed/qemu-ifup -device virtio-net,netdev=net0,mac=c0:01:0a:00:00:2a

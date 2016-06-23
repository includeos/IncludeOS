#! /bin/bash
export QEMU_EXTRA="-device virtio-net,netdev=net1 -netdev user,id=net1 -redir tcp:8002::80"
source ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh dualnic.img

#! /bin/bash
#! /bin/bash
set -e
sudo ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/create_bridge.sh
make -j

export CMDLINE="-append ${1}"
if [ $# -eq 0 ]
then
  export CMDLINE=""
fi
export QEMU_EXTRA="-device virtio-net,netdev=net1 -netdev user,id=net1 -redir tcp:8002::80"
source ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh `make kernelfile`

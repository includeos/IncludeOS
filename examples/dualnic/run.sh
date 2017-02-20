#! /bin/bash
set -e
INCLUDEOS_PREFIX=${INCLUDEOS_PREFIX-/usr/local}
sudo $INCLUDEOS_PREFIX/includeos/scripts/create_bridge.sh

FILE=$1
shift
export CMDLINE="-append ${1}"
if [ $# -eq 0 ]
then
  export CMDLINE=""
fi
export QEMU_EXTRA="-device virtio-net,netdev=net1 -netdev user,id=net1 -redir tcp:8002::80"
source $INCLUDEOS_PREFIX/includeos/scripts/run.sh $FILE ${*}

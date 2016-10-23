#! /bin/bash
set -e
sudo ../IncludeOS/etc/create_bridge.sh
make -j

export CMDLINE="-append ${1}"
if [ $# -eq 0 ]
then
  export CMDLINE=""
fi
export MEM_MAX=256
export MEM="-m 256"
source ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh `make kernelfile`

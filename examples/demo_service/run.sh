#! /bin/bash
set -e
sudo ${INCLUDEOS_PREFIX-/usr/local}/includeos/scripts/create_bridge.sh
make -j

export CMDLINE="-append ${1}"
if [ $# -eq 0 ]
then
  export CMDLINE=""
fi
source ${INCLUDEOS_PREFIX-/usr/local}/includeos/scripts/run.sh IncludeOS_example

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
export HDA="-kernel $FILE"
export MEM="-m 256" # we need a little bit of extra RAM to run these tests
source $INCLUDEOS_PREFIX/includeos/scripts/run.sh $FILE ${*}

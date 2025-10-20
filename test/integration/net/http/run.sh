#! /bin/bash
set -
INCLUDEOS_PREFIX=${INCLUDEOS_PREFIX-/usr/local}
sudo $INCLUDEOS_PREFIX/includeos/scripts/create_bridge.sh

FILE=$1
shift
source $INCLUDEOS_PREFIX/includeos/scripts/run.sh $FILE ${*}

#! /bin/bash
set -
INCLUDEOS_HOME=${INCLUDEOS_HOME-/usr/local}
sudo $INCLUDEOS_HOME/includeos/scripts/create_bridge.sh

FILE=$1
shift
source $INCLUDEOS_HOME/includeos/scripts/run.sh $FILE ${*}

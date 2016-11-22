#! /bin/bash
set -e
sudo ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/share/includeos/create_bridge.sh

FILE=$1
shift
source ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/share/includeos/run.sh $FILE ${*}

#! /bin/bash
set -e

sudo ../IncludeOS/etc/create_bridge.sh

make -j $1
source ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh `make servicefile`

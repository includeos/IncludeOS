#! /bin/bash
export SMP="-smp 4"
source ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh `make servicefile`

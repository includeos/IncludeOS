#! /bin/bash
export GRAPHICS="-vga std"
source ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh `make servicefile`

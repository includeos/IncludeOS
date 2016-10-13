#! /bin/bash
make
export MEM="-m 256" # we need a little bit of extra RAM to run these tests
source ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh `make servicefile`

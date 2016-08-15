#! /bin/bash
set -e
make -j $1
source ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh IRCd.img

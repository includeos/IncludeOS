#! /bin/bash
set -e

sudo ../IncludeOS/etc/create_bridge.sh

make -j $1
#export SERIAL="-serial file:server.log"
#export HDA="-kernel IRCd"
#export QEMU_EXTRA="-bios bios.bin"
time source ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh `make servicefile`

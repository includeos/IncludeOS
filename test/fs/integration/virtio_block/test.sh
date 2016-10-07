#!/bin/bash
make
./image.sh
export HDB="-drive file=./image.img,if=virtio,media=disk"
source ${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh `make servicefile`
./cleanup.sh

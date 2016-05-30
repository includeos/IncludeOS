#! /bin/bash
make -j
export HDB=" -drive file=./disk1.fat,if=virtio,media=disk "
${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh Acorn.img

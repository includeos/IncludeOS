#! /bin/bash
HDB="-drive file=disk.img,format=raw,if=virtio"

make -j
${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh Acorn.img

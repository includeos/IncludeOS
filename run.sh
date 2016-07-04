#! /bin/bash

# Make mnt/ if not exist
mkdir -p mnt

if ! mountpoint -q -- mnt/ ;
then
  echo "Mounting memdisk.fat"
  sudo mount -o rw memdisk.fat mnt/
  sync
fi

sudo cp -r disk1/. mnt/
sync
rm -f memdisk.o

make -j
export CPU=""
export HDB=" -drive file=./memdisk.fat,if=virtio,media=disk "
${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh Acorn.img

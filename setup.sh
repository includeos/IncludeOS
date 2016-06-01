#! /bin/bash
FAT_DISK=disk1.fat
if [ ! -f $FAT_DISK ];
then
  echo -e ">>> Creating FAT disk $FAT_DISK"
  fallocate -l 100000000 $FAT_DISK
  mkfs.fat $FAT_DISK
fi

mkdir -p mnt

echo -e ">>> Mounting $FAT_DISK in mnt/"
sudo mount -o rw $FAT_DISK mnt/

echo -e ">>> Copy content from disk1/ to $FAT_DISK"
sudo cp -r disk1/. mnt/

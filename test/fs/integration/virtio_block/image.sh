#!/bin/bash
echo "Creating *huge* disk for test"
truncate -s 4000000000 image.img
mkfs.fat image.img

mkdir -p mountpoint
sudo mount -o rw,sync image.img mountpoint

sudo cp service.cpp mountpoint/

sudo umount mountpoint
rmdir mountpoint
echo "Done"

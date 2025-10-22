#!/bin/bash
set -e #stop on first error
echo "Creating *huge* disk for test"
truncate -s 4000000000 image.img
sudo mkfs.fat image.img

mkdir -p mountpoint
sudo mount -o rw,sync image.img mountpoint

sudo cp service.cpp mountpoint/
sudo sync
sudo umount mountpoint
sudo rm -rf  mountpoint
echo "Done"

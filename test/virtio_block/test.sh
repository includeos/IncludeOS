#!/bin/bash
source ../test_base

make
export HDB="-drive file=./test.txt,if=virtio,media=disk"
start test_virtio_block.img

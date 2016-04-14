#!/bin/bash
source ../test_base

make
start test_virtio_queue.img

cd ../tcp
make clean && make
start test_tcp.img

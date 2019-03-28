#!/bin/bash
set -e
build_folder=$1
timeout=$2
shift 2
arguments=$@
source ${build_folder}/activate.sh
#sudo prior to timeout. in case its needed inside
sudo echo "sudo trigger"
timeout -s 2 $timeout python3 -u test.py $arguments || echo "Test timed out"; sleep 1

#!/bin/bash
set -e
timeout=$1
shift
arguments=$@
source activate.sh
#sudo prior to timeout. in case its needed inside
sudo echo "sudo trigger"

timeout -s 2 $timeout python2 -u test.py $arguments || echo "Test timed out"; sleep 1
source deactivate.sh

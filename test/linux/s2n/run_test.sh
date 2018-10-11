#!/bin/bash
set -e
export CC=gcc-7
export CXX=g++-7
$INCLUDEOS_PREFIX/bin/lxp-run &
JOB=$!
sleep 1
curl -k https://10.0.0.42 | grep '<title>IncludeOS Demo Service</title>'
ANSWER=$?
sudo killall `cat build/binary.txt`
printf "\n"

if [ $ANSWER == 0 ]; then
  echo ">>> Linux S2N stream test success!"
else
  exit 1
fi

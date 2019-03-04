#!/bin/bash

timeout=$1
shift
arguments=$@
timeout -s 2 $timeout python2 -u test.py $arguments || echo "Test timed out"; sleep 1

#!/bin/bash

for run in {1..32}
do
  ./fill.py &
#  ./test.py &
done

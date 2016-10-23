#!/bin/bash

for run in {1..16}
do
  ./fill.py &
#  ./test.py &
done

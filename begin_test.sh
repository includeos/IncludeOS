#!/bin/bash

for run in {1..10}
do
  ./test.py &
done

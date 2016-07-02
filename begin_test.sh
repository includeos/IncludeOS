#!/bin/bash

for run in {1..32}
do
  ./test.py &
done

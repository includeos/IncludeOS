#!/bin/bash

for run in {1..16}
do
  ./test.py &
done

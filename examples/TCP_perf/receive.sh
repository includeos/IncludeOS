#!/bin/bash
for i in `seq 1 4`;
do
    ncat 10.0.0.42 1337 --recv-only > bla.txt
done

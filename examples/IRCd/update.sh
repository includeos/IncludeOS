#!/bin/bash

#for i in `seq 1 100`:
#do
#  dd if=./build/IRCd bs=9000 > /dev/tcp/10.0.0.42/666
#  sleep 1
#done
dd if=build/IRCd bs=9000 > /dev/tcp/10.0.0.42/666

#!/bin/bash

#dd if=./IRCd.img bs=9000 > /dev/tcp/10.0.0.42/666
for i in `seq 1 100`:
do
  dd if=./LiveUpdate bs=9000 > /dev/tcp/10.0.0.42/666
  sleep 1
done

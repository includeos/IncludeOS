#!/bin/bash
while true;
do
  dd if=bigfile bs=9000 > /dev/tcp/10.0.0.42/1337
done

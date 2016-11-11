#!/bin/bash

#dd if=./IRCd.img bs=9000 > /dev/tcp/10.0.0.42/666
dd if=./LiveUpdate bs=9000 > /dev/tcp/10.0.0.42/666

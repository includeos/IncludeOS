#!/bin/bash

dd if=./LiveUpdate.img bs=9000 count=1000 > /dev/tcp/10.0.0.42/666

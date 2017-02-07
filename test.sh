#!/bin/bash

dd if=./build/LiveUpdate bs=9000 > /dev/tcp/10.0.0.42/666

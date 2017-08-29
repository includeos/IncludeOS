#!/bin/bash

dd if=./build/microlb bs=9000 > /dev/tcp/10.0.0.41/666

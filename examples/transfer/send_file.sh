#!/bin/bash
dd if=/dev/zero bs=1280 count=1048576 > /dev/tcp/10.0.0.42/81

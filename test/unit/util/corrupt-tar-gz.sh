#!/bin/bash
INFILE="$1"
OUTFILE="$2"

printf '\x0a\x0f\x0a\x0f\x0a\x0f\x0a\x0f' | dd conv=notrunc bs=1 of=$OUTFILE seek=$((`ls -nl $INFILE | awk '{print $5}'` - 8))

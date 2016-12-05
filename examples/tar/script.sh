#!/bin/bash

cp $1 input.bin

objcopy -I binary -O elf32-i386 -B i386 input.bin tarfile.o

rm input.bin

#!/bin/bash

# Takes first command line argument as parameter (f.ex. tar_example_folder.tar) and creates tarfile.o
# from this. tarfile.o is linked into the service in CMakeLists.txt

cp $1 input.bin

objcopy -I binary -O elf32-i386 -B i386 input.bin tarfile.o

rm input.bin

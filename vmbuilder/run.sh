#! /bin/bash
echo "Building system..."
make
./vmbuilder
echo "Starting VM: '$1'"
echo "-----------------------"

# Qemu with gdb debugging:
qemu-system-x86_64 -s -S -hda $1 -nographic
#bochs

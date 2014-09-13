#! /bin/bash

# GDB Under emacs: 
# M+x gdb
# $service.cpp + ENTER
# (gdb) file service
# (gdb) set non-stop off 
# (gdb) target remote localhost:1234
# (gdb) break _start
# (gdb) c

echo 
echo "Starting Gnu Debugger, using 'service' binary, connecting to localhost:1234"
echo "(Where qemu is supposed to be listening)"
gdb service -ex 'target remote localhost:1234'

#! /bin/bash

cd test/c_code
make
./c_cpp
cd ..

cd bootload
echo -e "Building VM with simple boot loader...\n"
make 
echo -e "Build complete \n"
echo -e "Starting VM with Qemu. "
echo -e "(Once inside Qemu, 'Ctrl+a c' will enter the Qemu console, from which you can type 'q' to quit.)\n"
echo -e "You should now get a boot message from the virtual machine:"
qemu-system-x86_64 -hda microMachine.hda -nographic

echo -e "\nTest complete. If you saw a boot message, it worked.\n"

cd ../..

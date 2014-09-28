#! /bin/bash

cd src
make 
sudo make install



cd ../seed

echo -e "Build complete \n"
echo -e "Starting VM with Qemu. "
echo -e "(Once inside Qemu, 'Ctrl+a c' will enter the Qemu console, from which you can type 'q' to quit.)\n"
echo -e "You should now get a boot message from the virtual machine:"
./run.sh IncludeOS_tests.img

echo -e "\nTest complete. If you saw a boot message, it worked.\n"

make clean

cd ../

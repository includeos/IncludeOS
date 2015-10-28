#! /bin/bash

pushd src
make -j4 
make test_service

echo -e "Build complete \n"
echo -e "Starting VM with Qemu. "
echo -e "(Once inside Qemu, 'Ctrl+a c' will enter the Qemu console, from which you can type 'q' to quit.)\n"
echo -e "You should now get a boot message from the virtual machine:"
./run.sh #`ls *.img -v | tail -n1`

echo -e "\nTest complete. If you saw a boot message, it worked.\n"

# Alfred: I'm using this for virtualbox testing 
if [ -d .vbox_share ]
then
    echo "Copying virtualbox image"
    cp *.vdi .vbox_share
fi

popd

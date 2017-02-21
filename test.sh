#! /bin/bash
. ./etc/set_traps.sh

pushd examples/demo_service
rm -rf build
mkdir -p build
pushd build
cmake ..
make

echo -e "Build complete \n"
echo -e "Starting VM with Qemu. "
echo -e "(Once inside Qemu, 'Ctrl+a c' will enter the Qemu console, from which you can type 'q' to quit.)\n"
echo -e "You should now get a boot message from the virtual machine:"
../run.sh build/IncludeOS_example.img

echo -e "\nTest complete. If you saw a boot message, it worked.\n"

popd

trap - EXIT

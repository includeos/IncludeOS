#! /bin/bash
. ./etc/set_traps.sh

export SYSTEM=`uname -s`

if [[ ! $SYSTEM =~ .*[L|l]inux.* ]]
then
  echo -e "\nRunning Solo5 / ukvm is currently only supported on Linux. \n"
  trap - EXIT
  exit 1
fi

pushd examples/demo_service
mkdir -p build
pushd build
PLATFORM=x86_solo5 cmake ..
make
popd
popd

echo -e "Build complete \n"
echo -e "Starting VM with ukvm/solo5. "

# The default ukvm-bin needs a disk, even if it's a dummy 0 byte one.
# If you want ukvm-bin with just the net module, you need to re-build it.
touch dummy.disk

# Create a tap100 device
sudo ./etc/scripts/ukvm-ifup.sh

# XXX: fix this during installation
chmod +x ./build_x86_64/precompiled/src/solo5_repo/ukvm/ukvm-bin

# XXX: should be using the installation directory instead
sudo ./build_x86_64/precompiled/src/solo5_repo/ukvm/ukvm-bin --disk=dummy.disk --net=tap100 ./examples/demo_service/build/IncludeOS_example

trap - EXIT

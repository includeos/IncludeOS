#! /bin/bash
. ./etc/set_traps.sh

export SYSTEM=`uname -s`

if [[ ! $SYSTEM =~ .*[L|l]inux.* ]]
then
  echo -e "\nRunning Solo5 is currently only supported on Linux. \n"
  trap - EXIT
  exit 1
fi

pushd examples/demo_service
mkdir -p build
boot --with-solo5-hvt .
popd
trap - EXIT

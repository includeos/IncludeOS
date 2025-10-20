#! /bin/bash
set -e # Exit immediately on error (we're trapping the exit signal)
trap 'previous_command=$this_command; this_command=$BASH_COMMAND' DEBUG
trap 'echo -e "\nINSTALL FAILED ON COMMAND: $previous_command\n"' EXIT

export SYSTEM=`uname -s`

if [[ ! $SYSTEM =~ .*[L|l]inux.* ]]
then
  echo -e "\nRunning Solo5 is currently only supported on Linux. \n"
  trap - EXIT
  exit 1
fi

pushd examples/demo_service
mkdir -p build
boot --with-solo5-spt .
popd
trap - EXIT

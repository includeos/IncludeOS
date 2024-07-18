#!/bin/bash

# A lot of these tests requires vmrunner and a network bridge.
# See https://github.com/includeos/vmrunner/pull/31
# for an explanation.

set -e
: "${QUICK_SMOKE:=}" # Define this to only do a ~1 min. smoke test.

success(){
  echo -e -n "\n👷💬 Step $1 succeeded "
  for ((i=1; i<=$1; i++)); do
    echo -n "👏"
  done
  echo ""
}

echo -e "\n🚧 1) Building and running unit tests"
nix-build unittests.nix
success 1

echo -e "\n🚧 2) Build the chainloader"
INCLUDEOS_CHAINLOADER=$(nix-build chainloader.nix)
echo "$INCLUDEOS_CHAINLOADER"
success 2

echo -e "\n🚧 3) Building the basic example"
nix-build example.nix
success 3

echo -e "\n🚧 4) Building and running a few key smoke tests"
nix-shell --argstr unikernel ./test/net/integration/udp --run ./test.py
nix-shell --argstr unikernel ./test/net/integration/tcp --run ./test.py
nix-shell --argstr unikernel ./test/kernel/integration/paging --run ./test.py
nix-shell --argstr unikernel ./test/kernel/integration/smp --run ./test.py
success 4

if [ -n "$QUICK_SMOKE" ]; then
  echo -e "\n👷💬 A lot of things are working! 💪"
  exit 0
fi

echo -e "\n🚧 5) Building and running all integration tests"

run_testsuite() {
  local base_folder="$1"
  shift
  local exclusion_list=("$@")

  echo -e "\n\n🚧 🚜 Running integration tests in $base_folder. Exceptions: "

  for exclude in "${exclusion_list[@]}"; do
    echo " - 📎 Skipping $exclude"
  done

  echo -e "\n======================================================================="

  for subfolder in "$base_folder"/*/; do
    local skip=false

    for exclude in "${exclusion_list[@]}"; do
      if [[ "$subfolder" == *"$exclude"* ]]; then
        skip=true
        break
      fi
    done

    if [ "$skip" = true ]; then
      continue
    fi

    echo -e "\n💣 Running tests in $subfolder "
    cmd="nix-shell --argstr unikernel $subfolder --run \"./test.py\""
    echo "👷💬 Reproduce with this 👇"
    echo "$cmd"
    echo -e "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
    $cmd
  done
}

# These tests should work or be removed, but are currently broken.
exclusions=(
  "LiveUpdate" # Missing includes
  "context"    # Outdated - references nonexisting OS::heap_end()
  "fiber"      # Crashes
  "grub"       # Fails with grub-mkrescue: error: xorriso not found.
  "modules"    # Requires 32-bit build, which our shell.nix is not set up for
)

run_testsuite "./test/kernel/integration" "${exclusions[@]}"
success 5

echo -e "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo -e "\n🌈✨ Everything is awesome ✨🦄"

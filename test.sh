#!/bin/bash

# A lot of these tests require vmrunner and a network bridge.
# See https://github.com/includeos/vmrunner/pull/31

set -e

: "${QUICK_SMOKE:=}" # Define this to only do a ~1 min. smoke test.
: "${DRY_RUN:=}"     # Define this to expand all steps without running any

success(){
  echo -e -n "\n👷💬 Step $1 succeeded "
  for ((i=1; i<=$1; i++)); do
    echo -n "👏"
  done
  echo ""
}

steps=1

run(){
  echo ""
  echo "🚧 Step $steps) $2"
  echo "⚙️  Running this command:"
  declare -f $1 | sed '1d;2d;$d' | sed 's/^[[:space:]]*//' # Print the function body
  echo "-------------------------------------- 💣 --------------------------------------"
  if [ ! $DRY_RUN ]
  then
    $1
  fi
  success $steps
  steps=$((steps + 1))
}

unittests(){
  nix-build unittests.nix
}

build_chainloader(){
  nix-build chainloader.nix
}

build_example(){
  nix-build example.nix
}

smoke_tests(){
  nix-shell --argstr unikernel ./test/net/integration/udp --run ./test.py
  nix-shell --argstr unikernel ./test/net/integration/tcp --run ./test.py
  nix-shell --argstr unikernel ./test/kernel/integration/paging --run ./test.py
  nix-shell --argstr unikernel ./test/kernel/integration/smp --run ./test.py
}

run unittests "Build and run unit tests"

run build_chainloader "Build the 32-bit chainloader"

run build_example "Build the basic example"

run smoke_tests "Build and run a few key smoke tests"


if [ -n "$QUICK_SMOKE" ]; then
  echo ""
  echo "👷💬 A lot of things are working! 💪"
  exit 0
fi

# Continuing from here will run all integration tests.

run_testsuite() {
  local base_folder="$1"
  shift
  local exclusion_list=("$@")
  substeps=1

  echo ""
  echo "====================================== 🚜 ======================================"
  echo ""
  echo "🚧 $steps) Running integration tests in $base_folder"

  if [ ! ${#exclusion_list[@]} -eq 0 ]
  then
    echo "⚠️  With the following exceptions: "
    for exclude in "${exclusion_list[@]}"; do
      echo " - ✏️  Skipping $exclude"
    done
  fi
  echo "--------------------------------------------------------------------------------"

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


    # The command to run, as string to be able to print the fully expanded command
    cmd="nix-shell --argstr unikernel $subfolder --run ./test.py"

    echo ""
    echo "🚧 Step $steps.$substeps"
    echo "📂 $subfolder"
    echo "⚙️  Running this command:"
    echo $cmd
    echo "-------------------------------------- 💣 --------------------------------------"


    if [ ! $DRY_RUN ]
    then
       $cmd
    fi

    substeps=$((substeps + 1))

  done

  success $steps
  steps=$((steps + 1))
}

#
# Kernel tests
#
exclusions=(
  "LiveUpdate" # Missing includes
  "context"    # Outdated - references nonexisting OS::heap_end()
  "fiber"      # Crashes
  "grub"       # Fails with grub-mkrescue: error: xorriso not found.
  "modules"    # Requires 32-bit build, which our shell.nix is not set up for
)

run_testsuite "./test/kernel/integration" "${exclusions[@]}"

#
# Networking tests
#
exclusions=(
  "dhclient"  # Times out because it requires DHCP server on the bridge.
  "dhcpd"     # Times out, requires certain routes to be set up. Seems easy.
  "dhcpd_dhclient_linux" # We can't run userspace tests with this setup yet.
  "gateway"   # Requires NaCl which is currently not integrated
  "http"      # Linking fails, undefined ref to http_parser_parse_url, http_parser_execute
  "microLB"   # Missing dependencies: microLB, diskbuilder, os_add_os_library
  "nat"       # Times out after 3 / 6 tests seem to pass. Might be a legit bug here.
  "router"    # Times out, requies sudo and has complex network setup.
  "router6"   # Times out: iperf3: error - unable to connect to server
  "vlan"      # Times out. Looks similar to the nat test - maybe similar cause?
  "websocket" # Linking fails, undefined ref to http_parser_parse_url, http_parser_execute
)

run_testsuite "./test/net/integration" "${exclusions[@]}"

echo -e "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo -e "\n🌈✨ Everything is awesome ✨\n"

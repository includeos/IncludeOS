#!/bin/bash

# Run all known IncludeOS tests.
#
# A lot of these tests require vmrunner and a network bridge.
# See https://github.com/includeos/vmrunner/pull/31

: "${QUICK_SMOKE:=}" # Define this to only do a ~1-5 min. smoke test.
: "${DRY_RUN:=}"     # Define this to expand all steps without running any
: "${CCACHE_FLAG:=}" # Define as "--arg withCcache true" to enable ccache.

steps=0
fails=0
failed_tests=()

success(){
  echo ""
  if [[ $1 =~ ^[0-9]+$ ]]; then
    echo -n "👷💬 Step $1 succeeded "
    for ((i=1; i<=$1; i++)); do
      echo -n "👏"
    done
  else
    echo "👷💬 Step $1 succeeded ✅"
  fi
  echo ""
}

fail(){
  echo ""
  echo "👷⛔ Step $1 failed ($2)"
  failed_tests+=("step $1: $2")
}

run(){
  steps=$((steps + 1))
  echo ""
  echo "🚧 Step $steps) $2"
  echo "⚙️  Running this command:"
  # This will print the body of a bash function, but won't expand variables
  # inside. It works well for bundling simple commands together and allows us to
  # print them without wrapping them in qotes.
  declare -f $1 | sed '1d;2d;$d' | sed 's/^[[:space:]]*//' # Print the function body
  echo "-------------------------------------- 💣 --------------------------------------"


  if [ ! $DRY_RUN ]
  then
    $1
  fi
  if [ $? -eq 0 ]; then
    success $steps
  else
    echo "‼️  Error: Command failed with exit status $?"
    fail $steps "$1"
    fails=$((fails + 1))
    return $?
  fi
}

unittests(){
  nix-build unittests.nix
}

build_chainloader(){
  nix-build $CCACHE_FLAG chainloader.nix
}

build_example(){
  nix-build $CCACHE_FLAG unikernel.nix
}

multicore_subset(){
  nix-build ./unikernel.nix --arg smp true $CCACHE_FLAG --argstr unikernel ./test/kernel/integration/smp --arg doCheck true

  # The following tests are not using multiple CPU's, but have been equippedd with some anyway
  # to make sure core functionality is not broken by missing locks etc. when waking up more cores.
  nix-shell ./unikernel.nix --arg smp true $CCACHE_FLAG --argstr unikernel ./test/net/integration/udp --arg doCheck true
  nix-build ./unikernel.nix --arg smp true $CCACHE_FLAG --argstr unikernel ./test/kernel/integration/paging --arg doCheck true
}

smoke_tests(){
  nix-build ./unikernel.nix $CCACHE_FLAG --argstr unikernel ./test/net/integration/udp
  nix-build ./unikernel.nix $CCACHE_FLAG --argstr unikernel ./test/net/integration/tcp
  nix-build ./unikernel.nix $CCACHE_FLAG --argstr unikernel ./test/kernel/integration/paging --arg doCheck true
  nix-build ./unikernel.nix $CCACHE_FLAG --argstr unikernel ./test/kernel/integration/smp --arg doCheck true
}

run unittests "Build and run unit tests"

run build_chainloader "Build the 32-bit chainloader"

run build_example "Build the basic example"

run multicore_subset "Run selected tests with multicore enabled"

if [ "$QUICK_SMOKE" ]; then

  run smoke_tests "Build and run a few key smoke tests"

  if [ $fails -eq 0 ]; then
    echo ""
    echo "👷💬 A lot of things are working! 💪"
  else
    echo ""
    echo "👷🧰 $fails / $steps steps failed. There's some work left to do. 🛠  "
    echo ""
    exit 1
  fi
  exit 0
fi

# Continuing from here will run all integration tests.

run_testsuite() {
  local base_folder="$1"
  shift
  local exclusion_list=("$@")

  steps=$((steps + 1))
  substeps=1
  subfails=0

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
    local sandboxed=true

    for exclude in "${exclusion_list[@]}"; do
      if [[ "$subfolder" == *"$exclude"* ]]; then
        skip=true
        break
      fi
    done
    if [ "$skip" = true ]; then
      continue
    fi

    for unsandbox in "${unsandbox_list[@]}"; do
      if [[ "$subfolder" == *"$unsandbox"* ]]; then
        sandboxed=false
        break
      fi
    done

    # The command to run, as string to be able to print the fully expanded command
    if $sandboxed; then
      cmd="nix-build ./unikernel.nix $CCACHE_FLAG --argstr unikernel ${subfolder%/} --arg doCheck true"
    else
      cmd="nix-shell ./unikernel.nix $CCACHE_FLAG --argstr unikernel ${subfolder%/} --arg doCheck true"
    fi

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
    if [ $? -eq 0 ]; then
      success "$steps.$substeps"
    else
      fail "$steps.$substeps" "$cmd"
      subfails=$((subfails + 1))
    fi

    substeps=$((substeps + 1))

  done

  if [ $subfails -eq 0 ]; then
    success $steps
  else
    fail $steps
    fails=$((fails + 1))
  fi
}

#
# Kernel tests
#
exclusions=(
  "LiveUpdate" # Missing includes
  "context"    # Outdated - references nonexisting OS::heap_end()
  "fiber"      # Crashes
  "modules"    # Requires 32-bit build, which our shell.nix is not set up for
)

unsandbox_list=(
  "term"  # fails to create the tun device, like the net integration tests
)
run_testsuite "./test/kernel/integration" "${exclusions[@]}"
unsandbox_list=()

#
# C++ STL runtime tests
#
exclusions=(

)

run_testsuite "./test/stl/integration" "${exclusions[@]}"


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

# all the following fail with the following error:
#   <vm> failed to create tun device: Operation not permitted
#   <vm> qemu-system-x86_64: -netdev bridge,id=net0,br=bridge43: bridge helper failed
unsandbox_list=(
  "./test/net/integration/configure"
  "./test/net/integration/icmp"
  "./test/net/integration/icmp6"
  "./test/net/integration/slaac"
  "./test/net/integration/tcp"
  "./test/net/integration/udp"
  "./test/net/integration/dns"  # except this one which times out instead
)
run_testsuite "./test/net/integration" "${exclusions[@]}"
unsandbox_list=()

echo -e "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"

if [ $fails -eq 0 ]; then
  echo ""
  echo "🌈✨ Everything is awesome ✨"
  echo ""
else
  echo ""
  echo "👷🧰 $fails / $steps steps failed. There's some work left to do. 🛠  "
  echo ""
  echo "Failed tests:"
  for t in "${failed_tests[@]}"; do
    echo "$t"
  done

  exit 1
fi

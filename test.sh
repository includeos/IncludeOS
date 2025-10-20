#!/usr/bin/env bash

# Run all known IncludeOS tests.
#
# A lot of these tests require vmrunner and a network bridge.
# See https://github.com/includeos/vmrunner/pull/31

: "${QUICK_SMOKE:=false}"         # Set to "true" for a ~1–5 min smoke test.
: "${DRY_RUN:=false}"             # Set to "true" to print steps without running them.
: "${USE_CCACHE:=false}"          # Set to "true" to enable ccache.
: "${TESTS_STDOUT:=/dev/stdout}"  # Set to /dev/null (or a file) to silence stdout of tests
: "${TESTS_STDERR:=/dev/stderr}"  # Set to /dev/null (or a file) to silence stderr of tests

#
# counters
#
steps=0
fails=0
failed_tests=()

#
# helpers
#
success() {
  if [[ $1 =~ ^[0-9]+$ ]]; then
    echo -n "👷💬 Step $1 succeeded "
    for ((i=1; i<=$1; i++)); do
      echo -n "👏"
    done
  else
    echo -n "👷💬 Step $1 succeeded ✅"
  fi
}

fail() {
  echo ""
  echo "👷⛔ Step $1 failed ($2)"
  failed_tests+=("step $1: $2")
}

#
# runners
#
run() {
  if [ "$DRY_RUN" = true ]; then
    echo "-------------------------------------- 💣 --------------------------------------"
    return;
  fi

  echo "-------------------------------------- 🗲 --------------------------------------"
  "${@}" >"${TESTS_STDOUT}" 2>"${TESTS_STDERR}"
  errno=$?
  echo "-------------------------------------- 󱦟 --------------------------------------"

  if [ $errno -eq 0 ]; then
    success "$steps.$substeps"
  else
    fail "$steps.$substeps" "${cmd[*]}"
  fi
  printf "\n\n\n"
}

run_single_test() {
    subfolder="$1"; shift

    cmd=(
      nix-shell
      --pure
      --arg withCcache "$USE_CCACHE"
      --argstr unikernel "$subfolder"
      --run "./test.py"
    )

    echo "⚙️  Running this command:"
    printf '%q ' "${cmd[@]}"
    printf '\n'

    run "${cmd[@]}"
    return $?
}

run_function() {
  steps=$((steps + 1))
  echo "🚧 Step ${steps}: $2"
  echo "⚙️  Running this command:"
  # This will print the body of a bash function, but won't expand variables
  # inside. It works well for bundling simple commands together and allows us to
  # print them without wrapping them in qotes.
  declare -f "$1" | sed '1d;2d;$d' | sed 's/^[[:space:]]*//' # Print the function body

  run "$1"
  return $?
}

run_testsuite() {
  local base_folder="$1"
  shift
  local exclusion_list=("$@")

  substeps=0
  subfails=0

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

    substeps=$((substeps + 1))
    echo "🚧 Step $steps.$substeps"
    echo "📂 $subfolder"

    if ! run_single_test "$subfolder"; then
      subfails=$((subfails + 1))
    fi

  done

  if [ $subfails -eq 0 ]; then
    success $steps
  else
    fail $steps
    fails=$((fails + 1))
  fi

  echo "Test suite finished (ran ${substeps} tests)"
  echo "--------------------------------------------------------------------------------"
}

#
# function suites
# used as targets for `./test.sh ...`
#
unittests() {
  nix-build unittests.nix
}

build_chainloader() {
  nix-build --arg withCcache "${USE_CCACHE}" chainloader.nix
}

build_example() {
  nix-build --arg withCcache "${USE_CCACHE}" example.nix
}

multicore_subset() {
  nix-shell --pure --arg smp true --arg withCcache "${USE_CCACHE}" --argstr unikernel ./test/kernel/integration/smp --run ./test.py

  # The following tests are not using multiple CPU's, but have been equippedd with some anyway
  # to make sure core functionality is not broken by missing locks etc. when waking up more cores.
  nix-shell --pure --arg smp true --arg withCcache "${USE_CCACHE}" --argstr unikernel ./test/net/integration/udp --run ./test.py
  nix-shell --pure --arg smp true --arg withCcache "${USE_CCACHE}" --argstr unikernel ./test/kernel/integration/paging --run ./test.py
}

smoke_tests() {
  nix-shell --pure --arg withCcache "${USE_CCACHE}" --argstr unikernel ./test/net/integration/udp --run ./test.py
  nix-shell --pure --arg withCcache "${USE_CCACHE}" --argstr unikernel ./test/net/integration/tcp --run ./test.py
  nix-shell --pure --arg withCcache "${USE_CCACHE}" --argstr unikernel ./test/kernel/integration/paging --run ./test.py
  nix-shell --pure --arg withCcache "${USE_CCACHE}" --argstr unikernel ./test/kernel/integration/smp --run ./test.py
}

#
# wrappers for testsuites
# also acts as wrappers for `./test.sh ...`
#
kernel_tests() {
  local exclusions=(
    "LiveUpdate" # Missing includes
    "context"    # Outdated - references nonexisting OS::heap_end()
    "fiber"      # Crashes
    "modules"    # Requires 32-bit build, which our shell.nix is not set up for
  )

  run_testsuite "./test/kernel/integration" "${exclusions[@]}"
}

stl_tests() {
  local exclusions=()

  run_testsuite "./test/stl/integration" "${exclusions[@]}"
}

net_tests() {
  local exclusions=(
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
}

custom_tests() {
  # add your custom tests here
  local exclusions=()

  # for testing all subdirectories in path
  : run_testsuite "./test/path/to/custom/tests*" "${exclusions[@]}"

  # for testing a single test
  : run_test "./test/path/to/single_test*"
}

#
# entry points
#

run_all() {
  #
  # unit tests
  #
  run_function unittests "Build and run unit tests"

  #
  # build tests
  #
  run_function build_chainloader "Build the 32-bit chainloader"

  run_function build_example "Build the basic example"

  run_function multicore_subset "Run selected tests with multicore enabled"

  if [ "$QUICK_SMOKE" = true ]; then

    run_function smoke_tests "Build and run a few key smoke tests"

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

  # all integration tests should go here

  run_function kernel_tests "Run kernel integration tests"

  run_function stl_tests "Run C++ STL integration tests"

  run_function net_tests "Run networking integration tests"

}

list_targets() {
  cat <<EOF
Available targets:
  unittests        build_chainloader     build_example
  multicore_subset smoke_tests           kernel_tests
  stl_tests        net_tests             custom_tests
EOF
}

main() {
  if [ $# -eq 0 ]; then
    run_all
  else
    for t in "$@"; do
      case "$t" in
        unittests|build_chainloader|build_example|multicore_subset|smoke_tests|kernel_tests|stl_tests|net_tests|custom_tests)
          run_function "$t" "Run target: $t"
          ;;
        all)
          run_all
          ;;
        list|-l|--list)
          list_targets
          ;;
        help|-h|--help)
          echo "Usage: $0 [target ...]"
          list_targets
          ;;
        *)
          echo "Unknown target: $t"
          list_targets;
          exit 2
          ;;
      esac
    done
  fi
}
main "$@"

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
    printf '%s\n' "$t"
  done

  exit 1
fi

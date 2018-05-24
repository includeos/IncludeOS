#!/bin/bash

# env variables
: ${INCLUDEOS_SRC:=$HOME/IncludeOS}
: ${INCLUDEOS_PREFIX:=/usr/local}
: ${CC:=""}
: ${CXX:=""}

###########################################################
# Compiler information:
############################################################
# This information can be changed to add/remove suport for a compiler version
CLANG_VERSION_MIN_REQUIRED="5.0"
cc_list="clang-7.0 clang-6.0 clang-5.0 clang"
cxx_list="clang++-7.0 clang++-6.0 clang++-5.0 clang++"

############################################################
# Support functions:
############################################################
# newer_than_minimum takes a specific compiler vesion and checks if it is compatible
# returns error if it is not compatible
newer_than_minimum() {
  local check=${1?You must specify which compiler to check}
  local CLANG_VERSION=${check: -3}
  if [[ $CLANG_VERSION < $CLANG_VERSION_MIN_REQUIRED ]]; then
    return 1
  fi
  return 0
}

check_installed() {
  local clang=${1?Clang version needs to be set}
  if [[ $(command -v $clang) ]]; then
    return 0
  fi
  return 1
}

# guess_compiler takes a list of compilers and returns the first one that is installed
check_if_in_list() {
  for compiler in $*; do
    if check_installed $compiler; then
      compiler_to_use=$compiler
      return 0
    fi
  done
  return 1
}

# containes checks if an item $1 is in the list supplied $2
contains() {
  for item in $2; do
    if [ "$1" == "$item" ]; then
      return 0
    fi
  done
  return 1
}

############################################################
# Main:
############################################################
# If CC and CXX is set then a check is performed to see if it compatible
#

if [ "$CC" != "" ]; then
  if ! newer_than_minimum "$CC"; then
    echo Your clang version: $CC is older than minimum required: $CLANG_VERSION_MIN_REQUIRED
  fi
  if ! contains "$CC" "$cc_list"; then
    echo Your clang version: $CC is not in the approved list: "$cc_list"
  fi
else
  check_if_in_list "$cc_list"
  if [ $? -eq 1 ]; then
    echo "None of the compatible CC's were installed: $cc_list"
  else
    echo Set CC=$compiler_to_use
    export CC=$compiler_to_use
  fi
fi

if [ "$CXX" != "" ]; then
  if ! newer_than_minimum "$CXX"; then
    echo Your clang version: $CXX is older than minimum required: $CLANG_VERSION_MIN_REQUIRED
  fi
  if ! contains "$CXX" "$cxx_list"; then
    echo Your clang version: $CXX is not in the approved list: "$cxx_list"
  fi
else
  check_if_in_list "$cxx_list"
  if [ $? -eq 1 ]; then
    echo "None of the compatible CC's were installed: $cxx_list"
  else
    echo Set CXX=$compiler_to_use
    export CXX=$compiler_to_use
  fi
fi

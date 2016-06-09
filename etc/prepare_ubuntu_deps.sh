# Prepare Ubuntu for installing dependencies
#
# Older versions don't have clang-3.8 in their apt source lists

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS/}
export clang_version=${clang_version-3.8}

UBUNTU_VERSION=`lsb_release -rs`
UBUNTU_CODENAME=`lsb_release -cs`

if [ $(echo "$UBUNTU_VERSION < 16.04" | bc) -eq 1 ]
then
  export clang_version=3.6
fi

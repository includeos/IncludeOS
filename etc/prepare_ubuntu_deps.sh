# Prepare Ubuntu for installing dependencies
#
# Older versions don't have clang-3.8 in their apt source lists

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS/}
clang_version=${clang_version-3.8}

UBUNTU_VERSION=`lsb_release -rs`
UBUNTU_CODENAME=`lsb_release -cs`

if [ $(echo "$UBUNTU_VERSION < 15.10" | bc) -eq 1 ]
then

  DEB_LINE="deb http://llvm.org/apt/$UBUNTU_CODENAME/ llvm-toolchain-$UBUNTU_CODENAME-$clang_version main"

  LINE_EXISTS=$(cat /etc/apt/sources.list | grep -c "$DEB_LINE")

  if [ $LINE_EXISTS -lt 1 ]
  then
    echo ">>> Adding clang $clang_version to your /etc/apt/sources.list"
    wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key|sudo apt-key add -

    echo $DEB_LINE | sudo tee -a /etc/apt/sources.list
    sudo apt-get update
  fi
fi

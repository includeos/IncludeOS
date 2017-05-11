#!/bin/bash

# Install a specific version of clang on a specific ubuntu version
clang_version=${1:-3.9}
ubuntu_version=${2:-$(lsb_release -cs)}

# Check if wanted version of clang is installed
if [[ $(command -v clang-$clang_version) && $(command -v clang++-$clang_version) ]]; then
	echo clang-$clang_version and clang++-$clang_version are already installed 
	exit 0
fi

# Construct apt string which is added to sources.list
export apt_string=$(printf "deb http://apt.llvm.org/%s/ llvm-toolchain-%s-%s main
deb-src http://apt.llvm.org/%s/ llvm-toolchain-%s-%s main" $ubuntu_version $ubuntu_version $clang_version $ubuntu_version $ubuntu_version $clang_version)

# Add to sources.list
sudo -E sh -c 'echo $apt_string >> /etc/apt/sources.list'

# Retrieve the archive signature
wget -qO - http://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -

# Update
sudo apt-get -qq update

# Install
sudo apt-get install -qqy clang-$clang_version

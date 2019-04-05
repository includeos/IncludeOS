#!/bin/bash
export PATH="$HOME/.local/bin:$PATH"
sudo locale-gen "en_US.UTF-8"
export LC_ALL="en_US.UTF-8"
sudo tee -a /etc/environment > /dev/null << EOT
LC_ALL="en_US.UTF-8"
EOT
#Install clang, nasm, cmake, qemu and bridge-utils
sudo apt install -y clang nasm cmake qemu bridge-utils

#Install pip, conan and python dependencies
sudo apt install -y python3-pip
pip3 install conan pystache antlr4-python3-runtime jsonschema psutil filemagic

#Install conan profiles and remotes
conan config install https://github.com/includeos/conan_config.git
export CONAN_DEFAULT_PROFILE_PATH="$HOME/.conan/profiles/clang-6.0-linux-x86_64"
sudo tee -a /etc/environment > /dev/null << EOT
CONAN_DEFAULT_PROFILE_PATH="$HOME/.conan/profiles/clang-6.0-linux-x86_64"
EOT

cd /IncludeOS
mkdir -p build
git checkout -b dev origin/dev
VERSION=`conan inspect -a version .. | cut -d " " -f 2`
conan editable add . includeos/$VERSION@includeos/latest --layout=etc/layout.txt
conan install -if build . 
conan build -bf build .


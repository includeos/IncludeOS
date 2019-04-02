#!/bin/bash

sudo locale-gen "en_US.UTF-8"
export LC_ALL="en_US.UTF-8"
env
sudo tee -a /etc/environment > /dev/null << EOT
LC_ALL="en_US.UTF-8"
EOT
export INCLUDEOS_PREFIX=/usr/local
export PATH=$PATH:$INCLUDEOS_PREFIX/includeos/bin
export CC="/usr/bin/clang-6.0"
export CXX="/usr/bin/clang++-6.0"
env
sudo tee -a /etc/environment > /dev/null << EOT
INCLUDEOS_PREFIX=/usr/local
CC="/usr/bin/clang-6.0"
CXX="/usr/bin/clang++-6.0"
EOT
env
sudo tee -a /etc/profile > /dev/null << EOT

PATH=$PATH:$INCLUDEOS_PREFIX/includeos/bin
EOT

#Install pip and conan using pip
sudo apt install -y python3-pip
pip3 install conan

#Install conan profiles and remotes
conan config install https://github.com/includeos/conan_config.git
conan profile list
conan profile show clang-6.0-linux-x86_64
conan remote list

#Install python dependencies
pip3 install pystache antlr4-python3-runtime
pip3 install jsonschema psutil filemagic

#Build includeos
cd ../..
pwd
conan create IncludeOS includeos/test -pr clang-6.0-linux-x86_64

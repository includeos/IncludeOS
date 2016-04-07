#! /bin/bash

# NOTE: This script requires emacs, and the emacs editorconfig plugin
#       It's loading the .emacs config located here:
#       https://github.com/alfred-bratterud/emacs.d

SRC=$1

for file in `find $SRC \( -not -path "*/cxxabi/*" -not -path "*/STREAM/*" -not -path "*/lest/*" -not -path "*/mod/*" \) -type f  \( -name *.cpp -or -name *.hpp -or -name *.inc \)`
do
  echo -e "\n >> Formatting $file"
  emacs $file -batch -l ~/.emacs.d/.emacs -f format-buffer
done

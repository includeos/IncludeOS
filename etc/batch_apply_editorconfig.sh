#! /bin/bash

SRC=$1

for file in `find $SRC \( -not -path "*/cxxabi/*" -not -path "*/STREAM/*" -not -path "*/lest/*" -not -path "*/mod/*" \) -type f  \( -name *.cpp -or -name *.hpp -or -name *.inc \)`
do
  echo -e "\n >> Formatting $file"
  emacs $file -batch -l ~/.emacs.d/.emacs -f format-buffer
done

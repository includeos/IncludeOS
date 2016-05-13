#! /bin/bash
if [ "$1" == "-onlyNames" ]
    then
    for t in `ls -d */`; do ./validate_test.py $t | grep PASS | rev | cut -d "/" -f1 | rev; done
    exit
fi

for t in `ls -d */`; do ./validate_test.py $t; done

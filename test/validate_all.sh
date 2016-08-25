#! /bin/bash
if [ "$1" == "-onlyNames" ]
    then
    for t in `ls -d */integration/*`; do ./validate_test.py $t | grep PASS | rev | cut -d " " -f1 | rev | while read x; do echo ${x##*/test/}; done; done
    exit
fi

for t in `ls -d */integration/*`; do ./validate_test.py $t; done

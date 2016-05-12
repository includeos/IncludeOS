#! /bin/bash
if [ $# -eq 0 ]
    then
    for t in `ls -d */`; do ./validate_test.py $t; done
    exit
fi

for t in `ls -d */`; do ./validate_test.py $t | grep PASS | rev | cut -d "/" -f1 | rev; done

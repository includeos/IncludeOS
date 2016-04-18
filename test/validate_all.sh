#! /bin/bash
for t in `ls -d */`; do ./validate_test.py $t; done

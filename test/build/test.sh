#!/bin/bash

# Script that builds all the files in the example folder
# Will print the output if the test fails

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}

errors_present=0

echo -e ">>> Will now attempt to make all examples. Outpt from make will only be present if an error occured"

for dir in `ls -d $INCLUDEOS_SRC/examples/*`
do
    cd $dir
    BASE=`basename $dir`
    echo -e "\n\n>>> Now making $BASE"
    { 
        make > /tmp/build_test 2>&1
    }|| { 
    errors_present=$((errors_present+1))
    cat /tmp/build_test 
    } 
done


# Clean up the files used
rm -f /tmp/build_test

exit $errors_present




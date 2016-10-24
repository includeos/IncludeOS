#!/bin/bash

# Script that checks for any binary blobs in the commit and raises errors if any are present

# IncludeOS_src directory
INCLUDEOS_SRC=${INCLUDEOS_SRC-~/IncludeOS}

# Make sure dev is fully updated and that we are standing in pull request
target_branch=${ghprbTargetBranch-dev}

# Figure out how many files were changed
files_changed=`git --no-pager diff --name-only FETCH_HEAD $(git merge-base FETCH_HEAD $target_branch)`

# Loop over files and check for binary blobs
for single_file in $files_changed; do
	filetype=`file -i $INCLUDEOS_SRC/$single_file`
    echo $filetype | grep -q charset=binary
    if [ $? -eq 0 ]; then
        echo Error: The file $INCLUDEOS_SRC/$single_file has been detected as a binary file
        exit 1
    fi
done

exit 0

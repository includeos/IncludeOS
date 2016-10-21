#!/bin/bash

# Script that checks for any binary blobs in the commit and raises errors if any are present

# Make sure dev is fully updated and that we are standing in pull request
target_branch=dev

# Figure out how many files were changed
files_changed=`git --no-pager diff --name-only FETCH_HEAD $(git merge-base FETCH_HEAD $target_branch)`

# Loop over files and check for binary blobs
for single_file in $files_changed; do
	filetype=`file -i $single_file`
    echo $filetype | grep -q charset=binary
    if [ $? -eq 0 ]; then
        echo Error: The file $single_file has been detected as a binary file
        exit 1
    fi
done

exit 0

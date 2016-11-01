#!/bin/bash

# Script that checks for any binary blobs in the commit and raises errors if any are present

# IncludeOS_src directory
INCLUDEOS_SRC=${INCLUDEOS_SRC-~/IncludeOS}

# Get the appropriate target branch
target_branch=${ghprbTargetBranch-dev}

# Make sure the original includeOS repo is compared against, temporary only
upstream=temp_blob_test
echo "### Fetching repo info from IncludeOS repo ###"
git remote add $upstream https://github.com/hioa-cs/IncludeOS.git
git remote update $upstream 

echo

# Create trap as cleanup
function finish {
	git remote remove $upstream
}
trap finish EXIT

# Figure out how many files were changed
files_changed=`git --no-pager diff --name-only HEAD $(git merge-base HEAD $upstream/$target_branch)`

binary_detected=0
# Loop over files and check for binary blobs
for single_file in $files_changed; do
	filetype=`file -i $INCLUDEOS_SRC/$single_file`
    echo $filetype | grep -q charset=binary
    if [ $? -eq 0 ]; then
        if [[ $single_file == *.* ]]; then
			continue
		fi
		let "binary_detected = $binary_detected + 1"
        echo Error: The file $INCLUDEOS_SRC/$single_file has been detected as a binary file
    fi
done

# Exit
echo "$binary_detected binary file(s) detected compared to branch $target_branch of IncludeOS"
if [ $binary_detected -gt 0 ]; then
	exit 1
else
	exit 0
fi

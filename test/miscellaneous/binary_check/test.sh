#!/bin/bash

# Script that checks for any binary blobs in the commit and raises errors if any are present

# Make sure dev is fully updated and that we are standing in pull request
target_branch=dev

# Figure out how many files were changed
files_changed=`git --no-pager diff --name-only FETCH_HEAD $(git merge-base FETCH_HEAD $target_branch)`

# Loop over files and check for binary blobs
for file in $files_changed; do


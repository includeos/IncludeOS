#!/bin/bash
. set_traps.sh
# qRename tag $1 to $2'

# 1) Create the new tag, as a copy of the old
# NOTE: You need to add in the old message in order to have git describe work.
MSG=`git tag -l -n100 $1 | cut -d' ' -f5-`

echo ">> Message: $MSG"

git tag $2 $1 -m"$MSG" --force

# 2) Delete the old tag
git tag -d $1                 

# 3) Push the deletion of the old tag
git push origin :refs/tags/$1   

# 4) Push the new tags
git push --tags       


trap - EXIT

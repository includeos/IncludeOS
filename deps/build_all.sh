#!/bin/bash

echo "Creating all conan packages:"


DIRS=`ls`
echo $DIRS
echo 

for DIR in $DIRS
do    
    if [ -d "$DIR" ]; then
	pushd $DIR
	echo "Creating package $DIR"
	conan create .
	popd
    fi
done
    
	  
	  

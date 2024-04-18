#!/bin/bash

echo "Creating all conan packages:"

# These must be built in order
PLATFORM=(musl
	  llvm_source
	  libunwind
	  libcxxabi
	  libcxx
	 )

dry_run=false
if [[ "$1" == "--dry" ]]
then
    dry_run=true
    echo "Dry run"
fi

echo "Building standard libs"
for DIR in "${PLATFORM[@]}"
do
    if [ -d "$DIR" ]; then
	pushd $DIR
	echo "Creating package $DIR"
	if [ "$dry_run" = false ]; then
	    conan create .
	fi
	popd
    fi
done

echo "Building everything else"
echo

ALL=(*)

for DIR in "${ALL[@]}"
do    
    if [[ -d "$DIR" && ! "${PLATFORM[*]}" =~ "$DIR" ]]; then
	pushd $DIR
	echo "Creating package $DIR"
	if [ "$dry_run" = false ]; then
	    conan create .
	fi
	popd
    fi
done
    
	  
	  

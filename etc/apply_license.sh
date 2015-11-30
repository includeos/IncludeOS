#!/bin/bash
licenselen=$(wc -l < license.txt)
extensions="hpp cpp h c"

# Enable recursive wildcards
shopt -s globstar
donotcheck="$(echo  ../src/crt/cxxabi/* ../src/include/cxxabi.h ../src/include/__cxxabi_config.h *sanos* ../doc ../mod/protobuf/include/**/* ../mod/SQLite/* ../examples/jansson/jansson/* ../examples/STREAM/stream.cpp ../src/elf/* ../examples/tcc/libtcc.h *poker.pb.h ../examples/protobuf/**/* ../mod/protobuf/api.pb.h ../api/utility/delegate.hpp)"

echo "IGNORE: $donotcheck"

license_txt=$(cat license.txt);

for extension in $extensions ; do
    echo "Processing extension $extension"
    file_list=$(find .. -iname "*.$extension")
    for line in $file_list; do 
	echo "Now processing file: $line"
	bypass=""
	for item in $donotcheck; do
	    [[ $line ==  $item ]] && bypass=1 && break;
	done
	
	# Break and continue if 
	[ ! -z $bypass ] && echo "Bypassing file" && continue
	

	current_head=$(head -n $licenselen $line)
	[[ $license_txt == $current_head ]] && echo "License OK" && continue;
	
	head -n 10 $line; 
	PS3='What would you like to do? (1-2)? '; 
	select answer in "Add license text from license.txt" "Do nothing and proceed to next file"; do
	    if [[ $answer = "Add license text from license.txt" ]]; 
	    then
		echo Adding license;
		( cat license.txt; echo; cat $line ) > /tmp/file; 
		mv /tmp/file $line 
		break; 
	    elif [[ $answer = "Do nothing and proceed to next file" ]]; then 
		echo Okay, doing nothing; 
		break; 
		
	    else 
		echo Please select one of the alternatives.
	    fi
	done		
    done
done

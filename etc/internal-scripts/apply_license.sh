#!/bin/bash
LICENSE_FILE=license_header.txt
LICENSE_FILE_OLD=license_header_old.txt
licenselen=$(wc -l < $LICENSE_FILE)
licenselen_old=$(wc -l < $LICENSE_FILE_OLD)
extensions="hpp cpp h c s"

SILENT=0
[[ $1 == "-s" ]] && SILENT=1 && echo "SILENT MODE"

# Enable recursive wildcards
shopt -s globstar
donotcheck="$(echo  ../src/crt/cxxabi/**/* ../src/include/cxxabi.h ../src/include/__cxxabi_config.h *sanos* ../doc ../mod/protobuf/include/**/* ../mod/SQLite/* ../examples/jansson/jansson/* ../examples/STREAM/stream.cpp ../src/elf/* ../examples/tcc/libtcc.h *poker.pb.h ../examples/protobuf/**/* ../mod/protobuf/api.pb.h ../api/utility/delegate.hpp ../src/crt/crtn.s ../src/crt/crti.s ../examples/tcc/setjmp.s)"

license_txt=$(cat $LICENSE_FILE);
license_txt_old=$(cat $LICENSE_FILE_OLD);

for extension in $extensions ; do
    echo "Processing extension $extension"
    file_list="$(find .. -iname "*.$extension") $(find ../api -maxdepth 1 -type f)"
    for line in $file_list; do 
	echo -e "\nFILE: $line"
	bypass=""
	for item in $donotcheck; do
	    [[ $line ==  $item ]] && bypass=1 && break;
	done
	
	# Break and continue if 
	[ ! -z $bypass ] && echo "Bypassing file" && continue
		
	# Replace old licenses
	current_head=$(head -n $(($licenselen_old)) $line) 
	if [[ $current_head == $license_txt_old ]] 
	then	    
	    echo "License is OLD. Replacing" 	    
	    ( cat $LICENSE_FILE; tail -n+$(($licenselen_old + 2)) $line ) > /tmp/file;
	    mv /tmp/file $line
	fi
	
	current_head=$(head -n $licenselen $line)	
	[[ $current_head == $license_txt  ]] && echo "License OK" && continue
	
	# If silent option, just fail on missing license (For jenkins)
	[[ $SILENT -eq 1 ]] && echo "ERROR: License missing or wrong. " && exit 42;
	
	head -n $licenselen $line; 
	PS3='What would you like to do? (1-2)? '; 
	select answer in "Add license text from $LICENSE_FILE" "Do nothing and proceed to next file"; do
	    if [[ $answer = "Add license text from $LICENSE_FILE" ]]; 
	    then
		echo Adding license;
		( cat $LICENSE_FILE; cat $line ) > /tmp/file; 
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

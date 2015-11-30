#!/bin/bash
licenselen=$(wc -l < license.txt)
extensions="hpp cpp h c"
donotcheck="./api/SQLite ./src/crt/cxxabi ./src/include/cxxabi.h ./src/include/__cxxabi_config.h *sanos* ./doc"
for extension in $extensions ; do
	for line in $(find .. -iname "*.$extension"); do 
	     echo "Now processing file: $line"
	     for item in $donotcheck; do
		if [[ $(echo $line | grep $item) ]] ; then
			echo "Bypassing file"; 
			break;
		fi
		head -n $licenselen $line > ./tmpfile
		if [[ $(diff license.txt ./tmpfile) ]]; then	
	    		 head -n 10 $line; 
			 PS3='What would you like to do? (1-2)? '; 
			 select answer in "Add license text from license.txt" "Do nothing and proceed to next file"; do
			 if [[ $answer = "Add license text from license.txt" ]]; then
				{ echo Adding license;
				( cat license.txt; echo; cat $line ) > /tmp/file; 
				mv /tmp/file $line 
				break; }
			elif [[ $answer = "Do nothing and proceed to next file" ]]; then {
				echo Okay, doing nothing; 
				break; }
			else {
				echo Please select one of the alternatives.
			} fi
	     	done
	
		else	
			echo "License OK";
			break;
		fi

	done
done
done

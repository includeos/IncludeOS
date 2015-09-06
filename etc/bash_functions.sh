function or_die {
    if [ $? -ne 0 ]; 
    then 
	echo $1". Exiting."; 
	exit -1; 
    fi
}

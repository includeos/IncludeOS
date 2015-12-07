#!/bin/bash 

if [ -z "$1" ]; then
echo "At least a new project name has to be given as a parameter: "
echo "./new.sh new_project"
exit 2
fi

TEMPLATE=$1
TEMPLATE=$(echo $TEMPLATE | tr ' ' '_')

FILENAME=${2:-$TEMPLATE}

echo "Modifying Makefile and run.sh with new project name"

sed -i "s/TEMPLATE/$TEMPLATE/g" Makefile
sed -i "s/START_FILE/${FILENAME}.cpp/g" Makefile
sed -i "s/TEMPLATE/$TEMPLATE/g" run.sh

if [ ! -f ${FILENAME}.cpp ]; then 
echo "Creating ${FILENAME}.cpp with default content"
echo "
#include <os>

void Service::start() {
  printf(\"Hello World from ${TEMPLATE}!\\n\");
}
" > $FILENAME.cpp
fi

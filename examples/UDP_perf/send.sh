#!/bin/bash
echo "Starting test.."

if [ -e send.txt ]
then
    echo "File exists"
else
    echo "Creating test file.."
    head -c 1G </dev/urandom > send.txt
fi

echo "Sending data in a loop..."

while [ 1 ]
do
    cat send.txt | ncat -u 10.0.0.42 1338 --send-only
done

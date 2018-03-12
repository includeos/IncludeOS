echo "Listening on port 1338 and dumping received data to recv.txt"
ncat -p 1338 -u 10.0.0.42 1337 --recv-only > recv.txt

# Rebuild IncludeOS
cd ~/IncludeOS/src/
make 

# Reinstall
sudo make install

# Back here, build and run
cd ~/IncludeOS/examples/sse
make
sudo ./run.sh IncludeOS_sse.img 


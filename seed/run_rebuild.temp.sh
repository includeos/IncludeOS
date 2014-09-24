# Rebuild IncludeOS
cd ~/IncludeOS/src/
make clean all

# Reinstall
sudo make install

# Back here, build and run
cd ~/IncludeOS/seed/
make
sudo ./run.sh IncludeOS_tests.img 

# Rebuild IncludeOS
cd ../src/
make clean all

# Reinstall
sudo make install

# Back here, build and run
cd ../seed/
make
./run.sh IncludeOS_tests.img 

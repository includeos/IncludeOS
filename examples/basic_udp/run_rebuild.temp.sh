# Rebuild IncludeOS
cd ~/IncludeOS/src/
make 

# Reinstall
sudo make install

# Back here, build and run
cd ~/IncludeOS/seed/
make
sudo ./run.sh IncludeOS_tests.img 

# Copy to virtual box shared folder (for running in virtualbox)
#cp ./run.sh IncludeOS_tests.img.vdi vbox_share

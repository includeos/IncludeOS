# Rebuild IncludeOS
cd ~/IncludeOS/src/
make 

# Reinstall
sudo make install

# Back here, build and run
cd ~/IncludeOS/examples/vga
make
sudo ./run.sh IncludeOS_VGA.img 

# Copy to virtual box shared folder (for running in virtualbox)
cp ./run.sh IncludeOS_VGA.img.vdi vbox_share

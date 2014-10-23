# Upgrade packages to solve dependencies

origdir=`pwd`

sudo apt-get clean
sudo apt-get dist-upgrade

bochs_version="2.6.6"
bochs_link="http://downloads.sourceforge.net/project/bochs/bochs/2.6.6/bochs-2.6.6.tar.gz?r=http%3A%2F%2Fsourceforge.net%2Fprojects%2Fbochs%2Ffiles%2Fbochs%2F2.6.6%2F&ts=1410367455&use_mirror=heanet"

bochs_file="bochs-$bochs_version.tar.gz"

# Create a directory (update this path if you don't like the location)
mkdir -p ~/src/
cd ~/src
wget -c --trust-server-name $bochs_link 
tar -xf bochs*.gz


# Bochs configure asks for this
sudo apt-get install -y  pkg-config
sudo apt-get install -y libgtk2.0-dev

# Bochs configuration:
# - Enable the internal debugger
# - Use X graphics; works over terminal
# - Enable USB (might be useful for USB-stick support)
# - Enable disassembly; sounded useful for assembly-parts of IncludeOS
cd bochs-$bochs_version
./configure --enable-debugger --with-x11 --enable-usb --enable-disasm 

# - I also tried using sdl-graphics for GUI (Ubuntu doesn't use X anymore):
#./configure --enable-debugger --with-sdl --enable-usb --enable-disasm 
# ... But this caused a linking error, so switched to x11, which works fine after all

#PATCH Makefile:
cat Makefile | sed s/lfreetype/"lfreetype -lpthread"/ > Makefile.tmp
mv Makefile.tmp Makefile

make
sudo make install

cd $origdir
cp .bochsrc ~/


echo -e "\nDONE! (hopefully)\n"

# Upgrade packages to solve dependencies
sudo apt-get clean
sudo apt-get dist-upgrade

# Bochs configure asks for this
sudo apt-get install pkg-config
sudo apt-get install libgtk2.0-dev

# Bochs configuration:
# - Enable the internal debugger
# - Use X graphics; works over terminal
# - Enable USB (might be useful for USB-stick support)
# - Enable disassembly; sounded useful for assembly-parts of IncludeOS
./configure --enable-debugger --with-x11 --enable-usb --enable-disasm 

# - I also tried using sdl-graphics for GUI (Ubuntu doesn't use X anymore):
#./configure --enable-debugger --with-sdl --enable-usb --enable-disasm 
# ... But this caused a linking error, so switched to x11, which works fine after all

echo "NOW UPDATE MAKEFILE: "
echo "Under 'LIBS', add '-lpthreads'"
echo "(Ref.: http://askubuntu.com/questions/376204/bochs-compiling-error-again)"
read INPUT

make
make install



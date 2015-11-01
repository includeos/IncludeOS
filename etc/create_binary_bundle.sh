INSTALL_DIR=/usr/local/IncludeOS
newlib=$INSTALL_DIR/i686-elf/lib


FOLDER="IncludeOS_bin"
OUTFILE="$FOLDER.tar.gz"

# Libraries
libc=$newlib/libc.a
libm=$newlib/libm.a
libcpp=$INSTALL_DIR/lib/libc++.a

## libgcc - required by newlib
GPP=$INSTALL_DIR/bin/i686-elf-g++
GCC_VER=`$GPP -dumpversion`
libgcc=$INSTALL_DIR/lib/gcc/i686-elf/$GCC_VER/libgcc.a

# Includes
include_newlib=$INSTALL_DIR/i686-elf/include
include_libcxx=$INSTALL_DIR/stdlib


# Make directory-tree
mkdir -p $FOLDER
mkdir -p $FOLDER/newlib
mkdir -p $FOLDER/libcxx
mkdir -p $FOLDER/libgcc
mkdir -p $FOLDER/crt

# Copy binaries
cp $libcpp $FOLDER/libcxx/
cp $libm $FOLDER/newlib/
cp $libc $FOLDER/newlib/
cp $libgcc $FOLDER/libgcc/
cp $INSTALL_DIR/crt/*.o $FOLDER/crt/

# Copy includes
cp -r $include_newlib $FOLDER/newlib/
cp -r $include_libcxx $FOLDER/libcxx/include

# Zip it
tar -czvf $OUTFILE $FOLDER

echo ">>> Bundle created as $FOLDER and gzipped into $OUTFILE"

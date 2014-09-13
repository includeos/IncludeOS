#! /bin/bash

OSDIR=/usr/local/IncludeOS/

echo ">>> Installing vmbuilder"
cd vmbuild
make
sudo cp vmbuild $OSDIR/

echo ">>> Installing IncludeOS"
cd ../src
make 
sudo make install

echo ">>> Done. Test the installation by running ./test.sh"


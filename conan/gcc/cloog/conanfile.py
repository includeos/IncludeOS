from conans import python_requires

base = python_requires("GnuBase/0.1/includeos/gnu")

class GccConan(base.GccConan):
    name = "mpfr"
    url = "https://www.gnu.org/software/%s"% name
    description = "The MPFR library is a C library for multiple-precision floating-point computations with correct rounding."
    license = "GNU GPL"


#ISL_VERSION=isl-0.12.2
#CLOOG_VERSION=cloog-0.18.1

#wget -nc ftp://gcc.gnu.org/pub/gcc/infrastructure/$ISL_VERSION.tar.bz2
#wget -nc ftp://gcc.gnu.org/pub/gcc/infrastructure/$CLOOG_VERSION.tar.gz

#wget -nc https://ftp.gnu.org/gnu/gcc/$GCC_VERSION/$GCC_VERSION.tar.gz

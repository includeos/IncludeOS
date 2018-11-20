from conans import python_requires

base = python_requires("GccBase/0.1/includeos/gcc")

class MpfrConan(base.GnuConan):
    name = "mpfr"
    url = "https://www.gnu.org/software/%s"% name
    description = "The MPFR library is a C library for multiple-precision floating-point computations with correct rounding."
    license = "GNU GPL"

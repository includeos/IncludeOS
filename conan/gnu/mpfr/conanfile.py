from conans import python_requires

base = python_requires("GnuBase/0.1/includeos/gnu")

class MpfrConan(base.GnuConan):
    name = "mpfr"
    url = "https://www.gnu.org/software/%s"% name
    description = "The MPFR library is a C library for multiple-precision floating-point computations with correct rounding."
    compression = 'xz'

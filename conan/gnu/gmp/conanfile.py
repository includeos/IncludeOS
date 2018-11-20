#from conans import ConanFile,tools
from conans import python_requires

base = python_requires("GnuBase/0.1/includeos/gnu")

class GmpConan(base.GnuConan):
    name = "gmp"
    url = "https://www.gnu.org/software/%s"% name
    description = "GMP is a free library for arbitrary precision arithmetic, operating on signed integers, rational numbers, and floating-point numbers."
    compression = 'xz'

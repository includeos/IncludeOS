from conans import python_requires

base = python_requires("GnuBase/0.1/includeos/gnu")

class MpcConan(base.GnuConan):
    name = "mpc"
    url = "https://www.gnu.org/software/%s"% name
    description = 'GNU MPC is a C library for the arithmetic of complex numbers with arbitrarily high precision and correct rounding of the result. It extends the principles of the IEEE-754 standard for fixed precision real floating point numbers to complex numbers, providing well-defined semantics for every operation'
    compression = 'gz'

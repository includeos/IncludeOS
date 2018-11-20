
from conans import tools,

#base = python_requires("GccBase/0.1/includeos/gcc")

class GccConan(ConanFile):
    settings = "os","arch","compiler"
    name = "gcc"
    url = "https://gcc.gnu.org/"
    description = "GCC, the GNU Compiler Collection"
    license = "GNU GPL"
    def build_requirements(self):

    def source(self):
        git = tools.Git()
        tag="gcc-"+self.version.replace('.','_')+"-release"
        git.clone("https://github.com/gcc-mirror/gcc.git",branch=tag)

    def build(self):
        
        ../$GCC_VERSION/configure --prefix=$INSTALL_PATH --target=$TARGET --enable-languages=c,c++ $CONFIGURATION_OPTIONS $NEWLIB_OPTION
make $PARALLEL_MAKE all-gcc
make install-gcc


import shutil

from conans import ConanFile,tools,AutoToolsBuildEnvironment

#base = python_requires("GccBase/0.1/includeos/gcc")

class GccConan(ConanFile):
    settings = "os","arch","compiler"
    name = "gcc"
    version="7.3.0"
    url = "https://gcc.gnu.org/"
    description = "GCC, the GNU Compiler Collection"
    license = "GNU GPL"
    def build_requirements(self):
        self.build_requires("binutils/2.31@%s/%s"%(self.user,self.channel))
        #TODO make these into optionals ?
        self.build_requires("mpfr/3.1.2@includeos/gcc")
        self.build_requires("mpfr/3.1.2@includeos/gcc")
        self.build_requires("gmp/6.1.2@includeos/gcc")
    def source(self):
        #git = tools.Git(folder="gcc")
        #tag="gcc-"+self.version.replace('.','_')+"-release"
        #git.clone("git://gcc.gnu.org/git/gcc.git",branch=tag)
        shutil.copytree("/home/kristian/repos/gcc", "gcc",symlinks=True)
    def build(self):
#CONFIGURATION_OPTIONS="--disable-multilib" # --disable-threads --disable-shared
#        ../$GCC_VERSION/configure --prefix=$INSTALL_PATH --target=$TARGET --enable-languages=c,c++ $CONFIGURATION_OPTIONS $NEWLIB_OPTION
#make $PARALLEL_MAKE all-gcc
#make install-gcc
        #args=["--disable-nls","--disable-werror"]
        compiler_args=['--disable-nls']
        compiler_args+=['--enable-languages=c,c++']
        compiler_args+=['--disable-multilib'] #not so sure about this one
        env_build = AutoToolsBuildEnvironment(self)
        env_build.configure(configure_dir="gcc",target=str(self.settings.arch)+"-elf",args=compiler_args) #what goes in here preferably
        #env_build.make()
        #env_build.install()

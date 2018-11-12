import shutil
from six import StringIO
from conans import ConanFile

class LibgccConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "libgcc"
    version = "1.0"
    license = 'GPL3'
    description = 'GNU compiler collection'
    url = "https://llvm.org/"

    def build(self):
        iobuf = StringIO()
        gcc=str(self.settings.arch)+"-pc-linux-gnu-gcc"
        self.run(gcc+" --print-libgcc-file-name", output=iobuf)
        src=iobuf.getvalue().rstrip('\n')
        print ("source "+src)
        #a bit nasty but it works
        shutil.copy(src,"./libcompiler.a")

    def package(self):
        self.copy("*.a",dst="libgcc",src="./")

    def deploy(self):
        self.copy("*.a",dst="libgcc",src="libgcc")

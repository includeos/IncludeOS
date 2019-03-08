import shutil
from six import StringIO
from conans import ConanFile

class LibgccConan(ConanFile):
    settings= "arch","os"
    name = "libgcc"
    version = "1.0"
    license = 'GPL3'
    description = 'GNU compiler collection'
    url = "https://llvm.org/"

    def build(self):
        iobuf = StringIO()
        extra=''
        if (str(self.settings.arch) in ["x86","armv8_32"]):
            extra="-m32"
        self.run("${CC} "+extra+" --print-libgcc-file-name", output=iobuf)
        src=iobuf.getvalue().rstrip('\n')
        print ("source "+src)
        #a bit nasty but it works
        shutil.copy(src,"./libcompiler.a")

    def package_info(self):
        self.cpp_info.libs=['compiler']
        #which compiler is in use doesnt really matter

    def package(self):
        self.copy("*.a",dst="lib",src="./")

    def deploy(self):
        self.copy("*.a",dst="lib",src="lib")

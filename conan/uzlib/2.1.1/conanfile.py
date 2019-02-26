import shutil
from conans import ConanFile,tools

class UzlibConan(ConanFile):
    settings="os","compiler","build_type","arch"
    name = "uzlib"
    version = "v2.1.1" #2.1.1 is probably the right one
    license = 'zlib'
    description = 'uzlib - Deflate/Zlib-compatible LZ77 compression/decompression library'
    url = "http://www.ibsensoftware.com/"
    exports_sources="Makefile.ios"
    def source(self):
        git = tools.Git(folder="uzlib")
        git.clone("https://github.com/pfalcon/uzlib",branch=str(self.version))
    ##hmm can i move this in configure..
    def configure(self):
        #this isnt c++ 
        del self.settings.compiler.libcxx
    def build(self):
        #a symlink would also do the trick
        shutil.copy("Makefile.ios","uzlib/src/Makefile")
        self.run("make -j20",cwd="uzlib/src")

    def package(self):
        self.copy("*.h",dst="include",src="uzlib/src")
        self.copy("*.a",dst="lib",src="uzlib/lib")

    def package_info(self):
        self.cpp_info.libs=['tinf']

    def deploy(self):
        self.copy("*.a",dst="lib",src="lib")
        self.copy("*.h",dst="include",src="include")

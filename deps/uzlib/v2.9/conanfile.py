import os
from conan import ConanFile,tools
from conan.tools.scm import Git
from conan.tools.files import copy

class UzlibConan(ConanFile):
    settings="os","compiler","build_type","arch"
    name = "uzlib"
    version = "v2.9" #2.1.1 is probably the right one
    license = 'zlib'
    description = 'uzlib - Deflate/Zlib-compatible LZ77 compression/decompression library'
    url = "http://www.ibsensoftware.com/"

    def source(self):
        repo = Git(self) 
        repo.clone("https://github.com/pfalcon/uzlib", target="uzlib")
        self.run("git fetch --all --tags --prune",cwd="uzlib")
        self.run("git checkout tags/"+str(self.version)+" -b "+str(self.version),cwd="uzlib")

    def build(self):
        self.run("make -j",cwd="uzlib/src")

    def package(self):
        source = os.path.join(self.source_folder, "uzlib", "src")
        uzlib  = os.path.join(self.source_folder, "uzlib")
        inc    = os.path.join(self.package_folder, "include")
        lib    = os.path.join(self.package_folder, "lib")
        copy(self, pattern="*.h", dst=inc, src=source)
        copy(self, pattern="*.a", dst=lib, src=uzlib)

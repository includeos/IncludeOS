import os
import shutil

from conan import ConanFile,tools
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import copy
from conan.tools.files import download
from conan.tools.files import unzip

class LLVMSourceConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "llvm_source"
    version = "7.0.1"
    license = 'NCSA','MIT'
    description = 'The LLVM Compiler Infrastructure Unwinder'
    url = "https://llvm.org/"
    
    def dirname(self): return "llvm-{}.src".format(self.version)

    def source(self):
        filename="{}-{}.src.tar.xz".format("llvm",self.version)
        download(self, "http://releases.llvm.org/{}/{}".format(self.version,filename),filename)
        unzip(self, filename)
        os.unlink(filename)        

    def package(self):
        src_path = os.path.join(self.source_folder, self.dirname())
        dst_path = os.path.join(self.package_folder, self.dirname())
        print("copying from {} to {}".format(src_path, dst_path))
        copy(self, pattern="*", src=src_path, dst=dst_path)

    def package_info(self):
        self.cpp_info.srcdirs = [self.dirname()]

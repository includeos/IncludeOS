import os
import shutil

from conan import ConanFile,tools
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import copy
from conan.tools.files import download
from conan.tools.files import unzip

class LibUnwindConan(ConanFile):
    #TODO check if the os matters at all here.. a .a from os a is compatible with os b
    settings= "compiler","arch","build_type","os"
    name = "libunwind"
    version = "7.0.1"
    license = 'NCSA','MIT'
    #version = [5.0.2,6.0.1,7.0.1] are known to be valid
    description = 'The LLVM Compiler Infrastructure Unwinder'
    url = "https://llvm.org/"

    options = {
        "shared":[True,False]
    }
    default_options = {
        "shared":False
    }
    no_copy_source=True

    def requirements(self):
        self.requires("llvm_source/{}".format(self.version))

    def configure(self):
        #we dont care what you had here youre building it :)
        del self.settings.compiler.libcxx


    def llvm_checkout(self,project):
        filename="{}-{}.src.tar.xz".format(project,self.version)
        download(self, "http://releases.llvm.org/{}/{}".format(self.version,filename),filename)
        unzip(self, filename)
        os.unlink(filename)
        shutil.move("{}-{}.src".format(project,self.version),project)


    def source(self):
        self.llvm_checkout("libunwind")


    def _triple_arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64",
            "armv8" : "aarch64"
        }.get(str(self.settings.arch))


    def generate(self):
        deps=CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        llvm_source=self.dependencies["llvm_source"].cpp_info.srcdirs[0]
        unwind_source=self.source_folder+"/libunwind"

        if (self.settings.compiler == "clang"):
            triple=self._triple_arch()+"-pc-linux-gnu"
            tc.variables["LIBUNWIND_TARGET_TRIPLE"] = triple

        # We currently have an implicit dependency on the host libc++
        # TODO: Use the LLVM source to find the C++ headers.
        # tc.variables["CMAKE_CXX_FLAGS"] = "-nostdlibinc"
        tc.variables['LIBUNWIND_ENABLE_SHARED']=self.options.shared
        tc.variables['LLVM_PATH']=llvm_source
        tc.variables['LLVM_ENABLE_LIBCXX']=True
        if (str(self.settings.arch) == "x86"):
            tc.variables['LLVM_BUILD_32_BITS']=True
        tc.generate()


    def build(self):
        source=self.source_folder+"/libunwind"
        cmake=CMake(self)
        cmake.configure(build_script_folder=source)
        cmake.build()


    def package(self):
        cmake=CMake(self)
        cmake.install()
        src_inc = os.path.join(self.source_folder, "libunwind", "include")
        pkg_inc = os.path.join(self.package_folder, "include")
        copy(self, pattern="*libunwind*.h", src=src_inc, dst=pkg_inc)


    def package_id(self):
        self.info.settings.os = "ANY"


    def package_info(self):
        self.cpp_info.includedirs=['include']
        self.cpp_info.libs=['unwind']
        self.cpp_info.libdirs=['lib']

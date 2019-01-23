import shutil #move
import os #unlink
from conans import ConanFile,tools,CMake

class LibUnwindConan(ConanFile):
    #TODO check if the os matters at all here.. a .a from os a is compatible with os b
    settings= "compiler","arch","build_type","os"
    name = "libunwind"
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

    def configure(self):
        #we dont care what you had here youre building it :)
        del self.settings.compiler.libcxx

    def llvm_checkout(self,project):
        filename="{}-{}.src.tar.xz".format(project,self.version)
        tools.download("http://releases.llvm.org/{}/{}".format(self.version,filename),filename)
        tools.unzip(filename)
        os.unlink(filename)
        shutil.move("{}-{}.src".format(project,self.version),project)

    def source(self):
        self.llvm_checkout("llvm")
        self.llvm_checkout("libunwind")

    def _triple_arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64",
            "armv8" : "aarch64"
        }.get(str(self.settings.arch))

    def _configure_cmake(self):
        cmake=CMake(self)
        llvm_source=self.source_folder+"/llvm"
        unwind_source=self.source_folder+"/libunwind"

        if (self.settings.compiler == "clang"):
            triple=self._triple_arch()+"-pc-linux-gnu"
            cmake.definitions["LIBUNWIND_TARGET_TRIPLE"] = triple

        cmake.definitions['LIBUNWIND_ENABLE_SHARED']=self.options.shared
        cmake.definitions['LLVM_PATH']=llvm_source
        if (str(self.settings.arch) == "x86"):
            cmake.definitions['LLVM_BUILD_32_BITS']=True
        cmake.configure(source_folder=unwind_source)
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        self.copy("*libunwind*.h",dst="include",src="libunwind/include")


    def package_info(self):
        self.cpp_info.includedirs=['include']
        self.cpp_info.libs=['unwind']
        self.cpp_info.libdirs=['lib']
        #where it was buildt doesnt matter
        self.info.settings.os="ANY"
        #what libcxx the compiler uses isnt of any known importance
        self.info.settings.compiler.libcxx="ANY"

    def deploy(self):
        self.copy("*.h",dst="include",src="include")
        self.copy("*.a",dst="lib",src="lib")

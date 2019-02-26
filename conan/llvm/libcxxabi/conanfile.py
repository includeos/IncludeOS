import os
import shutil

from conans import ConanFile,tools,CMake

class LibCxxAbiConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "libcxxabi"
    license = 'NCSA','MIT'
    description = 'The LLVM Compiler Infrastructure C++abi library'
    url = "https://llvm.org/"
    options ={
        "shared":[True,False]
    }
    default_options = {
        "shared":False
    }
    no_copy_source=True

    def llvm_checkout(self,project):
        filename="{}-{}.src.tar.xz".format(project,self.version)
        tools.download("http://releases.llvm.org/{}/{}".format(self.version,filename),filename)
        tools.unzip(filename)
        os.unlink(filename)
        shutil.move("{}-{}.src".format(project,self.version),project)

    def source(self):
        self.llvm_checkout("llvm")
        self.llvm_checkout("libcxx")
        self.llvm_checkout("libcxxabi")

    def _triple_arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64",
            "armv8" : "aarch64"
        }.get(str(self.settings.arch))


    def _configure_cmake(self):
        cmake=CMake(self)
        llvm_source=self.source_folder+"/llvm"
        source=self.source_folder+"/libcxxabi"
        unwind=self.source_folder+"/libunwind"
        libcxx=self.source_folder+"/libcxx"
        if (self.settings.compiler == "clang"):
            triple=self._triple_arch()+"-pc-linux-gnu"
            cmake.definitions["LIBCXXABI_TARGET_TRIPLE"] = triple
        cmake.definitions['LIBCXXABI_LIBCXX_INCLUDES']=libcxx+'/include'
        cmake.definitions['LIBCXXABI_USE_LLVM_UNWINDER']=True
        cmake.definitions['LIBCXXABI_ENABLE_SHARED']=self.options.shared
        cmake.definitions['LIBCXXABI_ENABLE_STATIC']=True
        #TODO consider that this locks us to llvm unwinder
        cmake.definitions['LIBCXXABI_ENABLE_STATIC_UNWINDER']=True
        cmake.definitions['LIBCXXABI_USE_LLVM_UNWINDER']=True
        cmake.definitions['LLVM_PATH']=llvm_source
        if (str(self.settings.arch) == "x86"):
            cmake.definitions['LIBCXXABI_BUILD_32_BITS']=True
            cmake.definitions['LLVM_BUILD_32_BITS']=True
        cmake.configure(source_folder=source)
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        self.copy("*.h",dst="include",src="libcxxabi/include")


    def package_info(self):
        #where it was buildt doesnt matter
        self.info.settings.os="ANY"
        #what libcxx the compiler uses isnt of any known importance
        self.info.settings.compiler.libcxx="ANY"
        self.cpp_info.includedirs=['include']
        self.cpp_info.libs=['c++abi']
        self.cpp_info.libdirs=['lib']


    def deploy(self):
        self.copy("*.h",dst="include",src="include")
        self.copy("*.a",dst="lib",src="lib")

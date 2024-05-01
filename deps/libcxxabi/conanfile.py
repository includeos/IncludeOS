import os
import shutil

from conan import ConanFile,tools
from conan.tools.scm import Git
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import copy
from conan.tools.files import download
from conan.tools.files import unzip

class LibCxxAbiConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "libcxxabi"
    version = "7.0.1"
    license = 'NCSA','MIT'
    description = 'The LLVM Compiler Infrastructure C++abi library'
    url = "https://llvm.org/"

    options ={
        "shared":[True,False]
    }
    default_options = {
        "shared":False
    }
    exports_sources=['files/float16_gcc.patch']
    no_copy_source=True

    def requirements(self):
        self.requires("llvm_source/{}".format(self.version))

    def llvm_checkout(self,project):
        filename="{}-{}.src.tar.xz".format(project,self.version)
        download(self, "http://releases.llvm.org/{}/{}".format(self.version,filename),filename)
        unzip(self, filename)
        os.unlink(filename)
        shutil.move("{}-{}.src".format(project,self.version),project)

    def source(self):
        self.llvm_checkout("libcxx")
        self.llvm_checkout("libcxxabi")
        # TODO move to build step?
        # Forbidden in conan 2.2.2
        # ConanException: 'self.settings' access in 'source()' method is forbidden
        # if (self.settings.compiler == "gcc"):
        #     tools.patch("libcxx",patch_file='files/float16_gcc.patch')


    def _triple_arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64",
            "armv8" : "aarch64"
        }.get(str(self.settings.arch))

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        llvm_source=self.dependencies["llvm_source"].cpp_info.srcdirs[0]
        source=self.source_folder+"/libcxxabi"
        unwind=self.source_folder+"/libunwind"
        libcxx=self.source_folder+"/libcxx"

        if (self.settings.compiler == "clang"):
            triple=self._triple_arch()+"-pc-linux-gnu"
            tc.variables["LIBCXXABI_TARGET_TRIPLE"] = triple
        tc.variables['LIBCXXABI_LIBCXX_INCLUDES']=libcxx+'/include'
        tc.variables['LIBCXXABI_USE_LLVM_UNWINDER']=True
        tc.variables['LIBCXXABI_ENABLE_SHARED']=self.options.shared
        tc.variables['LIBCXXABI_ENABLE_STATIC']=True
        #TODO consider that this locks us to llvm unwinder
        tc.variables['LIBCXXABI_ENABLE_STATIC_UNWINDER']=True
        tc.variables['LIBCXXABI_USE_LLVM_UNWINDER']=True
        tc.variables['LLVM_ENABLE_LIBCXX']=True
        tc.variables['LLVM_PATH']=llvm_source
        if (str(self.settings.arch) == "x86"):
            tc.variables['LIBCXXABI_BUILD_32_BITS']=True
            tc.variables['LLVM_BUILD_32_BITS']=True

        tc.generate()


    def build(self):
        #cmake = self._configure_cmake()
        source=self.source_folder+"/libcxxabi"
        cmake=CMake(self)
        cmake.configure(build_script_folder=source)
        cmake.build()

    def package(self):
        #cmake = self._configure_cmake() <- Replaced by generate
        cmake = CMake(self)
        cmake.install()
        source = os.path.join(self.source_folder, "libcxxabi", "include")
        inc    = os.path.join(self.package_folder, "include")
        copy(self, pattern = "*.h",dst=inc, src=source)



    def package_info(self):
        #where it was buildt doesnt matter
        #self.info.settings.os="ANY"
        #what libcxx the compiler uses isnt of any known importance
        #self.info.settings.compiler.libcxx="ANY"
        #
        # NOTE 2024: 👆 Those are illegal in conan 2.2.2
        # self.info.settings.os="ANY"
	# -> ConanException: 'self.info' access in 'package_info()' method is forbidden
        # It builds fine without it, but not sure what the consequences are.

        self.cpp_info.includedirs=['include']
        self.cpp_info.libs=['c++abi']
        self.cpp_info.libdirs=['lib']

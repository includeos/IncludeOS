import os
import shutil

from conans import ConanFile,tools,CMake

class LibCxxConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "libcxx"
    generators="cmake"
    license = 'NCSA','MIT'
    description = 'The LLVM Compiler Infrastructure C++ library'
    url = "https://llvm.org/"
    options ={
        "shared":[True,False],
        "threads":[True,False]
    }
    default_options = {
        "shared":False,
        "threads":True
    }
    exports_sources="CMakeLists.txt"
    no_copy_source=True

    def requirements(self):
        self.requires("musl/v1.1.18@{}/{}".format(self.user,self.channel))
        self.requires("libunwind/{}@{}/{}".format(self.version,self.user,self.channel))
        self.requires("libcxxabi/{}@{}/{}".format(self.version,self.user,self.channel))

    def imports(self):
        self.copy("*cxxabi*",dst="include",src="include")

    def llvm_checkout(self,project):
        filename="{}-{}.src.tar.xz".format(project,self.version)
        tools.download("http://releases.llvm.org/{}/{}".format(self.version,filename),filename)
        tools.unzip(filename)
        os.unlink(filename)
        shutil.move("{}-{}.src".format(project,self.version),project)

    def source(self):
        self.llvm_checkout("llvm")
        self.llvm_checkout("libcxx")
        shutil.copy("libcxx/CMakeLists.txt","libcxx/CMakeListsOriginal.txt")
        shutil.copy("CMakeLists.txt","libcxx/CMakeLists.txt")

    def _triple_arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64",
            "armv8" : "aarch64"
        }.get(str(self.settings.arch))

    def _configure_cmake(self):
        cmake=CMake(self)
        llvm_source=self.source_folder+"/llvm"
        source=self.source_folder+"/libcxx"

        cmake.definitions['CMAKE_CROSSCOMPILING']=True
        cmake.definitions['LIBCXX_HAS_MUSL_LIBC']=True
        cmake.definitions['LIBCXX_ENABLE_THREADS']=self.options.threads
        #TODO consider how to have S_LIB
        cmake.definitions['LIBCXX_HAS_GCC_S_LIB']=False
        cmake.definitions['LIBCXX_ENABLE_STATIC']=True
        cmake.definitions['LIBCXX_ENABLE_SHARED']=self.options.shared
        cmake.definitions['LIBCXX_ENABLE_STATIC_ABI_LIBRARY']=True
        #TODO consider using this ?
        #cmake.definitions['LIBCXX_STATICALLY_LINK_ABI_IN_STATIC_LIBRARY']=True
        cmake.definitions['LIBCXX_CXX_ABI']='libcxxabi'
        cmake.definitions["LIBCXX_INCLUDE_TESTS"] = False
        cmake.definitions["LIBCXX_LIBDIR_SUFFIX"] = ''
        cmake.definitions['LIBCXX_SOURCE_PATH']=source
        #TODO figure out how to do this with GCC ? for the one case of x86_64 building x86 code
        if (self.settings.compiler == "clang"):
            triple=self._triple_arch()+"-pc-linux-gnu"
            cmake.definitions["LIBCXX_TARGET_TRIPLE"] = triple
        cmake.definitions['LLVM_PATH']=llvm_source
        if (str(self.settings.arch) == "x86"):
            cmake.definitions['LLVM_BUILD_32_BITS']=True
        #cmake.configure(source_folder=source)
        cmake.configure(source_folder=source)
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()


    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        self.copy("*cxxabi*",dst="include",src="include")

    def package_info(self):
        #this solves a lot but libcxx still needs to be included before musl
        self.cpp_info.includedirs = ['include','include/c++/v1']
        self.cpp_info.libs=['c++','c++experimental']
        self.cpp_info.libdirs=['lib']
        #where it was buildt doesnt matter
        self.info.settings.os="ANY"
        #what libcxx the compiler uses isnt of any known importance
        self.info.settings.compiler.libcxx="ANY"

    def deploy(self):
        self.copy("*",dst="include",src="include")
        self.copy("*.a",dst="lib",src="lib")

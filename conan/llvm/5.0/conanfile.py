#binutils recepie first take!!
#todo figure out to get a build directory ?
#todo use shutil to move versioned to unversioned ?

import os
import shutil

from conans import ConanFile,tools,CMake

class LlvmConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "llvm"
    version = "5.0"
    branch = "release_%s"% version.replace('.','')
    license = 'NCSA','MIT'
    description = 'The LLVM Compiler Infrastructure'
    url = "https://llvm.org/"
    exports_sources=['../../../api*posix*']
    no_copy_source=True

    def build_requirements(self):
        self.build_requires("binutils/2.31@includeos/test")
        self.build_requires("musl/v1.1.18@includeos/test")

    def imports(self):
        self.copy("*",dst="target",src=".")

    def llvm_checkout(self,project):
        llvm_project=tools.Git(folder="llvm/projects/"+project)
        llvm_project.clone("https://github.com/llvm-mirror/%s.git"%project,branch=self.branch)

    def source(self):
        llvm = tools.Git(folder="llvm")
        llvm.clone("https://github.com/llvm-mirror/llvm.git",branch=self.branch)
        self.llvm_checkout("libcxxabi")
        self.llvm_checkout("libcxx")
        self.llvm_checkout("libunwind")
        #TODO verify the need for this
        shutil.copytree("api/posix","posix")
    def build(self):
        triple = str(self.settings.arch)+"-pc-linux-gnu"
        threads='OFF'
        cmake = CMake(self)

        llvm_source=self.source_folder+"/llvm"
        musl=self.build_folder+"/target/include"

        #TODO get these from profile
        #shouldnt the CFLAGS come from somewhere more sane like the profile we are building for ?
        cflags="-msse3 -g -mfpmath=sse"
        cxxflags=cflags

        if (self.settings.compiler == "clang" ):
            cflags+=" -nostdlibinc" # do this in better python by using a list
        if (self.settings.compiler == "gcc" ):
            cflags+=" -nostdinc"
        #doesnt cmake have a better way to pass the -I params ?

        cmake.definitions["CMAKE_C_FLAGS"] =cflags+" -D_LIBCPP_HAS_MUSL_LIBC -I"+musl+" -I"+llvm_source+"/posix -I"+llvm_source+"/projects/libcxx/include -I"+llvm_source+"/projects/libcxxabi/include -Itarget/include"
        cmake.definitions["CMAKE_CXX_FLAGS"] =cxxflags+" -D_LIBCPP_HAS_MUSL_LIBC -I"+musl+" -I"+llvm_source+"/posix -I"+llvm_source+"/projects/libcxx/include -I"+llvm_source+"/projects/libcxxabi/include"

        cmake.definitions["LLVM_ABI_BREAKING_CHECKS"]='FORCE_OFF'
        cmake.definitions["CMAKE_EXPORT_COMPILE_COMMANDS"] = 'ON'
        cmake.definitions["BUILD_SHARED_LIBS"] = 'OFF'
        cmake.definitions["CMAKE_EXPORT_COMPILE_COMMANDS"] = 'ON'
        cmake.definitions["TARGET_TRIPLE"] = triple
        cmake.definitions["LLVM_DEFAULT_TARGET_TRIPLE"] =triple
        cmake.definitions["LLVM_INCLUDE_TESTS"] = 'OFF'
        cmake.definitions["LLVM_ENABLE_THREADS"] = threads


        #from gentoo ebuild
        cmake.definitions["LIBCXXABI_LIBDIR_SUFFIX"] = ''
        cmake.definitions["LIBCXXABI_INCLUDE_TESTS"] = 'OFF'


        cmake.definitions["LIBCXXABI_TARGET_TRIPLE"] = triple
        cmake.definitions["LIBCXXABI_ENABLE_THREADS"] = threads
        cmake.definitions["LIBCXXABI_HAS_PTHREAD_API"] = threads
        cmake.definitions["LIBCXXABI_USE_LLVM_UNWINDER"] = 'ON'
        cmake.definitions["LIBCXXABI_ENABLE_STATIC_UNWINDER"] = 'ON'

        #from gentoo ebuild..
        cmake.definitions["LIBCXX_INCLUDE_TESTS"] = 'OFF'
        cmake.definitions["LIBCXX_LIBDIR_SUFFIX"] = ''
        cmake.definitions["LIBCXX_HAS_GCC_S_LIB"] = 'OFF'
        cmake.definitions["LIBCXX_CXX_ABI_INCLUDE_PATHS"]="libcxxabi/include"

        cmake.definitions["LIBCXX_ENABLE_STATIC"] = 'ON'
        cmake.definitions["LIBCXX_ENABLE_SHARED"] = 'OFF'
        cmake.definitions["LIBCXX_CXX_ABI"] = 'libcxxabi'

        cmake.definitions["LIBCXX_ENABLE_THREADS"] = threads
        cmake.definitions["LIBCXX_TARGET_TRIPLE"] = triple
        cmake.definitions["LIBCXX_ENABLE_STATIC_ABI_LIBRARY"] = 'ON'
        cmake.definitions["LIBCXX_HAS_MUSL_LIBC"]='ON'
        cmake.definitions["LIBCXX_CXX_ABI_LIBRARY_PATH"] = 'lib/'

        cmake.definitions["LIBUNWIND_TARGET_TRIPLE"] = triple
        cmake.definitions["LIBUNWIND_ENABLE_SHARED"] = 'OFF'

        cmake.configure(source_folder=llvm_source)
        cmake.build(target='unwind')
        cmake.build(target='cxxabi')
        cmake.build(target='cxx')

    def package(self):
        self.copy("*",dst="include/c++",src="include/c++")
        self.copy("*.a",dst="lib",src="lib")

    def deploy(self):
        self.copy("*",dst="include/c++",src="include/c++")
        self.copy("*.a",dst="lib",src="lib")

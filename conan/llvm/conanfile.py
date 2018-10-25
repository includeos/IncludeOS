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

    #release_50 <-- branch name
    license = 'NCSA','MIT'
    #requires = 'binutils/2.31@includeos/test'
    description = 'The LLVM Compiler Infrastructure'
    url = "https://llvm.org/"
    ## ?? exports_sources=['../../etc*musl*musl.patch', '../../etc*musl*endian.patch','../../api*syscalls.h','../../etc*musl*syscall.h']
    ##TODO enable if treads is set in profile!!
    treads='OFF'
    #for speedup doing multibuild
    #no_copy_source=True
    #for debugging
    #keep_imports=True
    def build_requirements(self):
        self.build_requires("binutils/2.31@includeos/test")
        self.build_requires("musl/v1.1.18@includeos/test")

    def llvm_checkout(self,project):
        llvm_project=tools.Git(folder="llvm/projects/"+project)
        llvm_project.clone("https://github.com/llvm-mirror/%s.git"%project,branch=self.branch)

    def source(self):
        llvm = tools.Git(folder="llvm")
        llvm.clone("https://github.com/llvm-mirror/llvm.git",branch=self.branch)
        self.llvm_checkout("libcxxabi")
        self.llvm_checkout("libcxx")
        self.llvm_checkout("libunwind")

    def build(self):
        triple = str(self.settings.arch)+"-elf"

        cmake = CMake(self)
        #doesnt cmake have a better way to pass the -I params ?
        #shouldnt the CFLAGS come from somewhere more sane like the profile we are building for ?
        cmake.definitions["DCMAKE_CXX_FLAGS"] ="-std=c++14 -msse3 -g -mfpmath=sse -nostdlibinc -D_LIBCPP_HAS_MUSL_LIBC -Illvm/projects/libcxx/include -Illvm/projects/libcxxabi/include"
        ##TODO make an on / off function for simple flags ?
        cmake.definitions["CMAKE_EXPORT_COMPILE_COMMANDS"] = 'ON'
        cmake.definitions["BUILD_SHARED_LIBS"] = 'OFF'
        cmake.definitions["CMAKE_EXPORT_COMPILE_COMMANDS"] = 'ON'
        cmake.definitions["TARGET_TRIPLE"] = self.triple
        cmake.definitions["LLVM_DEFAULT_TARGET_TRIPLE"] =self.triple
        cmake.definitions["LLVM_INCLUDE_TESTS"] = 'OFF'
        cmake.definitions["LLVM_ENABLE_THREADS"] = self.threads

        cmake.definitions["LIBCXXABI_TARGET_TRIPLE"] = self.triple
        cmake.definitions["LIBCXXABI_ENABLE_THREADS"] = self.treads
        cmake.definitions["LIBCXXABI_HAS_PTHREAD_API"] = self.treads
        cmake.definitions["LIBCXXABI_USE_LLVM_UNWINDER"] = 'ON'
        cmake.definitions["LIBCXXABI_ENABLE_STATIC_UNWINDER"] = 'ON'

        cmake.definitions["LIBCXX_ENABLE_STATIC"] = 'ON'
        cmake.definitions["LIBCXX_ENABLE_SHARED"] = 'OFF'
        cmake.definitions["LIBCXX_CXX_ABI"] = 'libcxxabi'
        cmake.definitions["LIBCXX_ENABLE_THREADS"] = self.threads
        cmake.definitions["LIBCXX_TARGET_TRIPLE"] = self.triple
        cmake.definitions["LIBCXX_ENABLE_STATIC_ABI_LIBRARY"] = 'ON'
        cmake.definitions["LIBCXX_CXX_ABI_LIBRARY_PATH"] = 'lib/'

        cmake.definitions["LIBUNWIND_TARGET_TRIPLE"] = self.triple
        cmake.definitions["LIBUNWIND_ENABLE_SHARED"] = 'OFF'

        cmake.configure()
        cmake.build(target='libunwind.a')
        cmake.build(target='libc++abi.a')
        cmake.build(target='libc++.a')

    def package(self):
        print("TODO")
    #    self.copy("*.h",dst="include",src="include")
    #    self.copy("*.a",dst="lib",src="lib")

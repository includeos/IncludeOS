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
    exports_sources=['../../api*posix*']
    #for speedup doing multibuild
    no_copy_source=True
    #for debugging
    keep_imports=True

    def build_requirements(self):
        self.build_requires("binutils/2.31@includeos/test")
        self.build_requires("musl/v1.1.18@includeos/test")

    def imports(self):
        self.copy("*",dst="target",src=".")
        #triple = str(self.settings.arch)+"-elf"
        #tgt=triple+"-elf"
        #self.copy("*",dst="bin",src="bin") #copy binaries..
        #self.copy("*.a",dst="lib",src="lib")
        #self.copy("*",dst=tgt,src=tgt)

    def llvm_checkout(self,project):
        llvm_project=tools.Git(folder="llvm/projects/"+project)
        llvm_project.clone("https://github.com/llvm-mirror/%s.git"%project,branch=self.branch)

    def source(self):
        #llvm = tools.Git(folder="llvm")
        #llvm.clone("https://github.com/llvm-mirror/llvm.git",branch=self.branch)
        #self.llvm_checkout("libcxxabi")
        #self.llvm_checkout("libcxx")
        #self.llvm_checkout("libunwind")
        shutil.copytree("/home/kristian/repos/llvm", "llvm",symlinks=True)
        shutil.copytree("/home/kristian/repos/libcxx","llvm/projects/libcxx",symlinks=True)
        shutil.copytree("/home/kristian/repos/libcxxabi","llvm/projects/libcxxabi",symlinks=True)
        shutil.copytree("/home/kristian/repos/libunwind","llvm/projects/libunwind",symlinks=True)

        shutil.copytree("api/posix","posix")
        #shutil.copy("etc/musl/syscall.h","musl/src/internal")
        #self.copy("*",dst="llvm",src="/home/kristian/repos/llvm")
        #self.copy("*",dst="llvm/libcxx",src="/home/kristian/repos/libcxx")
        #self.copy("*",dst="llvm/libcxxabi",src="/home/kristian/repos/libcxxabi")
        #self.copy("*",dst="llvm/libunwind",src="/home/kristian/repos/libunwind")
    def build(self):
        triple = str(self.settings.arch)+"-pc-linux-gnu"
        #str(self.settings.arch)+"-elf"
        threads='OFF'
        cmake = CMake(self)

        llvm_source=self.source_folder+"/llvm"
        musl=self.build_folder+"/target/include"
        #nostdinc"nostdinc"
        #if (self.settins.compiler == "clang")

        #doesnt cmake have a better way to pass the -I params ?
        #shouldnt the CFLAGS come from somewhere more sane like the profile we are building for ?
        #cmake.definitions["CMAKE_CXX_FLAGS"] ="-std=c++14 -msse3 -g -mfpmath=sse -nostdlibinc -D_LIBCP_HAS_MUSL_LIBC -Illvm/projects/libcxx/include -Illvm/projects/libcxxabi/include"
        cmake.definitions["CMAKE_C_FLAGS"] =" -msse3 -g -mfpmath=sse  -I"+musl+"  -nostdinc -nostdlib -D_LIBCPP_HAS_MUSL_LIBC -I"+llvm_source+"/posix -I"+llvm_source+"/projects/libcxx/include -I"+llvm_source+"/projects/libcxxabi/include -Itarget/include"
        cmake.definitions["CMAKE_CXX_FLAGS"] ="-std=c++14 -msse3 -g -mfpmath=sse -D_LIBCPP_HAS_MUSL_LIBC -I"+musl+"  -I"+llvm_source+"/posix -I"+llvm_source+"/projects/libcxx/include -I"+llvm_source+"/projects/libcxxabi/include"
        ##TODO make an on / off function for simple flags ?
        #cmake.definitions["CMAKE_CXX_LINK_FLAGS"]="-Wl,-rpath,/usr/lib/gcc/x86_64-pc-linux-gnu -L/usr/lib/gcc/x86_64-pc-linux-gnu/8.2.0/libstdc++.so"
        ##for clang to find libstdc++
        #cmake.definitions["CMAKE_CXX_LINK_FLAGS"]="-Wl,-rpath,/usr/lib/gcc/x86_64-pc-linux-gnu/7.3.0 -L/usr/lib/gcc/x86_64-pc-linux-gnu/7.3.0"
        #cmake.definitions["CMAKE_CROSSCOMPILING"] = 'ON'
        cmake.definitions["LLVM_ABI_BREAKING_CHECKS"]='FORCE_OFF'
        cmake.definitions["CMAKE_EXPORT_COMPILE_COMMANDS"] = 'ON'
        cmake.definitions["BUILD_SHARED_LIBS"] = 'OFF'
        cmake.definitions["CMAKE_EXPORT_COMPILE_COMMANDS"] = 'ON'
        cmake.definitions["TARGET_TRIPLE"] = triple
        cmake.definitions["LLVM_DEFAULT_TARGET_TRIPLE"] =triple
        cmake.definitions["LLVM_INCLUDE_TESTS"] = 'OFF'
        cmake.definitions["LLVM_ENABLE_THREADS"] = threads

        cmake.definitions["LIBCXXABI_TARGET_TRIPLE"] = triple
        cmake.definitions["LIBCXXABI_ENABLE_THREADS"] = threads
        cmake.definitions["LIBCXXABI_HAS_PTHREAD_API"] = threads
        cmake.definitions["LIBCXXABI_USE_LLVM_UNWINDER"] = 'ON'
        cmake.definitions["LIBCXXABI_ENABLE_STATIC_UNWINDER"] = 'ON'

        cmake.definitions["LIBCXX_ENABLE_STATIC"] = 'ON'
        cmake.definitions["LIBCXX_ENABLE_SHARED"] = 'OFF'
        cmake.definitions["LIBCXX_CXX_ABI"] = 'libcxxabi'
        cmake.definitions["LIBCXX_ENABLE_THREADS"] = threads
        cmake.definitions["LIBCXX_TARGET_TRIPLE"] = triple
        cmake.definitions["LIBCXX_ENABLE_STATIC_ABI_LIBRARY"] = 'ON'
        cmake.definitions["LIBCXX_CXX_ABI_LIBRARY_PATH"] = 'lib/'

        cmake.definitions["LIBUNWIND_TARGET_TRIPLE"] = triple
        cmake.definitions["LIBUNWIND_ENABLE_SHARED"] = 'OFF'

        cmake.configure(source_folder=llvm_source)
        cmake.build(target='unwind')
        cmake.build(target='cxxabi')
        cmake.build(target='cxx')
        #cmake.install()

    def package(self):
        self.copy("*",dst="include/c++",src="include/c++")
        self.copy("*.a",dst="lib",src="lib")
        #print("TODO")
    #    self.copy("*.h",dst="include",src="include")
    #    self.copy("*.a",dst="lib",src="lib")

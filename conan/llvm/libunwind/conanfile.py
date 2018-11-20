import os
import shutil

from conans import ConanFile,tools,CMake

class LibUnwindConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "libunwind"
    #version = "7.0" #todo remove..

    license = 'NCSA','MIT'
    description = 'The LLVM Compiler Infrastructure Unwinder'
    url = "https://llvm.org/"
    options ={"shared":[True,False],"threads":[True,False]}
    default_options = {"shared":False,"threads":True}
    #exports_sources=['../../../api*posix*']
    no_copy_source=True

    def llvm_checkout(self,project):
        branch = "release_%s"% self.version.replace('.','')
        llvm_project=tools.Git(folder=project)
        llvm_project.clone("https://github.com/llvm-mirror/%s.git"%project,branch=self.branch)

    def source(self):
        self.llvm_checkout("llvm")
        self.llvm_checkout("libunwind")

    def _configure_cmake(self):
        cmake=CMake(self)
        llvm_source=self.source_folder+"/llvm"
        unwind_source=self.source_folder+"/libunwind"
        #threads=self.options.threads
        if (self.settings.compiler == "clang"):
            triple=str(self.settings.arch)+"-pc-linux-gnu"
            cmake.definitions["LIBUNWIND_TARGET_TRIPLE"] = triple

        cmake.definitions['LIBUNWIND_ENABLE_SHARED']=self.options.shared
        cmake.definitions['LLVM_PATH']=llvm_source
        cmake.configure(source_folder=unwind_source)
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        self.copy("*libunwind*.h",dst="include",src="libunwind/include")

    def deploy(self):
        self.copy("*.h",dst="include",src="include")
        self.copy("*.a",dst="lib",src="lib")

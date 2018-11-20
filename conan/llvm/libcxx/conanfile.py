import os
import shutil

from conans import ConanFile,tools,CMake

class LibCxxConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "libcxx"
    generators="cmake"
    license = 'NCSA','MIT'
    description = 'The LLVM Compiler Infrastructure Unwinder'
    url = "https://llvm.org/"
    options ={"shared":[True,False],"threads":[True,False]}
    default_options = {"shared":False,"threads":True}
    no_copy_source=True

    def build_requirements(self):
        self.build_requires("musl/v1.1.18@includeos/test")
        self.build_requires("libunwind/{}@{}/{}".format(self.version,self.user,self.channel))
        self.build_requires("libcxxabi/{}@{}/{}".format(self.version,self.user,self.channel))

    def imports(self):
        #lazy way to get libunwind and libcxx into place
        self.copy("*h",src="include",dst="include")
        self.copy("*.a",dst="lib",src="lib")

    def llvm_checkout(self,project):
        branch = "release_%s"% self.version.replace('.','')
        llvm_project=tools.Git(folder=project)
        llvm_project.clone("https://github.com/llvm-mirror/%s.git"%project,branch=branch)

    def source(self):
        self.llvm_checkout("llvm")
        self.llvm_checkout("libcxx")
        
        component='libcxx'
        if not os.path.exists(os.path.join(self.source_folder,
                                               component,
                                               "CMakeListsOriginal.txt")):
                shutil.move(os.path.join(self.source_folder,
                                         component,
                                         "CMakeLists.txt"),
                            os.path.join(self.source_folder,
                                         component,
                                         "CMakeListsOriginal.txt"))
                with open(os.path.join(self.source_folder,
                                       component,
                                       "CMakeLists.txt"), "w") as cmakelists_file:
                    cmakelists_file.write("cmake_minimum_required(VERSION 2.8)\n"
                                          "include(\"${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake\")\n"
                                          #libcxx/include MUST be before the musl include #include_next
                                          "include_directories("+self.source_folder+"/"+component+"/include)\n"
                                          #"conan_basic_setup the manual way\n"
                                          "include_directories(\"${CONAN_INCLUDE_DIRS_LIBCXXABI}\")\n"
                                          "include_directories(\"${CONAN_INCLUDE_DIRS_LIBUNWIND}\")\n"
                                          "include_directories(\"${CONAN_INCLUDE_DIRS_MUSL}\")\n"
                                          "include(CMakeListsOriginal.txt)\n")

    def _configure_cmake(self):
        cmake=CMake(self)
        llvm_source=self.source_folder+"/llvm"
        #cxxabi=self.source_folder+"/libcxxabi"
        #unwind=self.source_folder+"/libunwind"
        source=self.source_folder+"/libcxx"

        cmake.definitions['CMAKE_CROSSCOMPILING']=True
        cmake.definitions['LIBCXX_HAS_MUSL_LIBC']=True
        cmake.definitions['LIBCXX_ENABLE_THREADS']=self.options.threads
        cmake.definitions['LIBCXX_HAS_GCC_S_LIB']=False
        cmake.definitions['LIBCXX_ENABLE_STATIC']=True
        cmake.definitions['LIBCXX_ENABLE_SHARED']=self.options.shared
        cmake.definitions['LIBCXX_ENABLE_STATIC_ABI_LIBRARY']=True

        cmake.definitions['LIBCXX_CXX_ABI']='libcxxabi'
        cmake.definitions['LIBCXX_CXX_ABI_INCLUDE_PATHS']='include'
        #instead of using the CONAN_PATH
        cmake.definitions["LIBCXX_CXX_ABI_LIBRARY_PATH"]='lib/'
        cmake.definitions["LIBCXX_INCLUDE_TESTS"] = False
        cmake.definitions["LIBCXX_LIBDIR_SUFFIX"] = ''
        #TODO
        if (self.settings.compiler == "clang"):
            triple=str(self.settings.arch)+"-pc-linux-gnu"
            cmake.definitions["LIBCXX_TARGET_TRIPLE"] = triple
        cmake.definitions['LLVM_PATH']=llvm_source
        cmake.configure(source_folder=source)
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        #maybe use shutil copy !!
        #shutil.copytree(self.package_folder+"include/c++/v1",self.package_folder+"include")
        #self.copy("*",dst="include",src="include/c++/v1")
    def package_info(self):
        self.cpp_info.includedirs = ['include/c++/v1']
    def deploy(self):
        self.copy("*",dst="include",src="include")
        self.copy("*.a",dst="lib",src="lib")

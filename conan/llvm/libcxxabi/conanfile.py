from conans import ConanFile,tools,CMake

class LibCxxAbiConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "libcxxabi"
    license = 'NCSA','MIT'
    description = 'The LLVM Compiler Infrastructure Unwinder'
    url = "https://llvm.org/"
    options ={
        "shared":[True,False]
    }
    default_options = {
        "shared":False
    }
    no_copy_source=True

    def requirements(self):
        self.requires("libunwind/{}@{}/{}".format(self.version,self.user,self.channel))

    def imports(self):
        self.copy("*.a",dst="lib",src="lib")

    def llvm_checkout(self,project):
        branch = "release_{}".format(self.version.replace('.',''))
        llvm_project=tools.Git(folder=project)
        llvm_project.clone("https://github.com/llvm-mirror/{}.git".format(project),branch=branch)

    def source(self):
        self.llvm_checkout("llvm")
        self.llvm_checkout("libcxx")
        self.llvm_checkout("libcxxabi")

    def _configure_cmake(self):
        cmake=CMake(self)
        llvm_source=self.source_folder+"/llvm"
        source=self.source_folder+"/libcxxabi"
        unwind=self.source_folder+"/libunwind"
        libcxx=self.source_folder+"/libcxx"
        if (self.settings.compiler == "clang"):
            triple=str(self.settings.arch)+"-pc-linux-gnu"
            cmake.definitions["LIBCXXABI_TARGET_TRIPLE"] = triple
        cmake.definitions['LIBCXXABI_LIBCXX_INCLUDES']=libcxx+'/include'
        cmake.definitions['LIBCXXABI_USE_LLVM_UNWINDER']=True
        cmake.definitions['LIBCXXABI_ENABLE_SHARED']=self.options.shared
        cmake.definitions['LIBCXXABI_ENABLE_STATIC']=True
        #TODO consider that this locks us to llvm unwinder
        cmake.definitions['LIBCXXABI_ENABLE_STATIC_UNWINDER']=True
        cmake.definitions['LIBCXXABI_USE_LLVM_UNWINDER']=True
        cmake.definitions['LLVM_PATH']=llvm_source
        cmake.configure(source_folder=source)
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        self.copy("*.h",dst="include",src="libcxxabi/include")

    def deploy(self):
        self.copy("*.h",dst="include",src="include")
        self.copy("*.a",dst="lib",src="lib")

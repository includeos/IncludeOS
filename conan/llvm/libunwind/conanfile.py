from conans import ConanFile,tools,CMake

class LibUnwindConan(ConanFile):
    #TODO check if the os matters at all here.. a .a from os a is compatible with os b
    settings= "compiler","arch","build_type","os"
    name = "libunwind"
    license = 'NCSA','MIT'
    description = 'The LLVM Compiler Infrastructure Unwinder'
    url = "https://llvm.org/"
    options = {
        "shared":[True,False]
    }
    default_options = {
        "shared":False
    }
    no_copy_source=True

    def llvm_checkout(self,project):
        branch = "release_{}".format(self.version.replace('.',''))
        llvm_project=tools.Git(folder=project)
        llvm_project.clone("https://github.com/llvm-mirror/{}.git".format(project),branch=branch)

    def source(self):
        self.llvm_checkout("llvm")
        self.llvm_checkout("libunwind")

    def _triple_arch(self):
        if str(self.settings.arch) == "x86":
            return "i386"
        return str(self.settings.arch)
    def _configure_cmake(self):
        cmake=CMake(self)
        llvm_source=self.source_folder+"/llvm"
        unwind_source=self.source_folder+"/libunwind"

        if (self.settings.compiler == "clang"):
            triple=self._triple_arch()+"-pc-linux-gnu"
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


    def package_info(self):
        #where it was buildt doesnt matter
        self.info.settings.os="ANY"
        #what libcxx the compiler uses isnt of any known importance
        self.info.settings.compiler.libcxx="ANY"

    def deploy(self):
        self.copy("*.h",dst="include",src="include")
        self.copy("*.a",dst="lib",src="lib")

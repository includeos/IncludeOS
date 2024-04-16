import os
import shutil

from conan import ConanFile,tools
from conan.tools.scm import Git
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import copy
from conan.tools.files import download
from conan.tools.files import unzip

class LibCxxConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "libcxx"
    default_user = "includeos"
    version = "7.0.1"

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
    exports_sources= ['CMakeLists.txt', 'linux/version.h', 'patches/float16_gcc.patch']
    no_copy_source=True

    def requirements(self):
        self.requires("musl/[>=1.1.18]")
        self.requires("llvm_source/{}".format(self.version))
        self.requires("libunwind/{}".format(self.version))
        self.requires("libcxxabi/{}".format(self.version))

    def imports(self):
        self.copy("*cxxabi*",dst="include",src="include")

    def llvm_checkout(self,project):
        filename="{}-{}.src.tar.xz".format(project,self.version)
        download(self, "http://releases.llvm.org/{}/{}".format(self.version,filename),filename)
        unzip(self, filename)
        os.unlink(filename)
        shutil.move("{}-{}.src".format(project,self.version),project)

    def source(self):
        self.llvm_checkout("libcxx")

        # print("Overwriting LLVM's CMakeLists.txt with our own");

        # The reason for doing this is to be able add our own include paths
        # the recommended way seems to be to use CMake's find_package and let
        # conan generate what's needed for those for each of the dependencies.
        #
        # However, it doesn't look like it works and it could be because
        # find_package doesn't take effect without target_link_libraries, which
        # it seems we can't use since we're not buidling a normal cmake target.
        # 
        # Instead, adding include paths directly with -I via CMAKE_CXX_FLAGS
        # 
        # shutil.copy("libcxx/CMakeLists.txt","libcxx/CMakeListsOriginal.txt")
        # shutil.copy("CMakeLists.txt","libcxx/CMakeLists.txt")

    def _triple_arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64",
            "armv8" : "aarch64"
        }.get(str(self.settings.arch))


    def generate(self):
        deps=CMakeDeps(self)
        deps.generate()        
        tc = CMakeToolchain(self)
        llvm_source=self.dependencies["llvm_source"].cpp_info.srcdirs[0]
        source=self.source_folder+"/libcxx"

        musl_path = self.dependencies["musl"].package_folder
        musl_inc = os.path.join(musl_path, self.dependencies["musl"].cpp_info.includedirs[0])

        print("Musl path: " + musl_path)
        
        
        tc.variables["CMAKE_CXX_FLAGS"] = "-nostdlibinc -I" + musl_inc + " -I" + self.source_folder
        tc.variables['CMAKE_CROSSCOMPILING']=True
        tc.variables['LIBCXX_HAS_MUSL_LIBC']=True
        tc.variables['_LIBCPP_HAS_MUSL_LIBC']=True
        tc.variables['LIBCXX_ENABLE_THREADS']=self.options.threads
        tc.variables['LIBCXX_HAS_GCC_S_LIB']=False
        tc.variables['LIBCXX_ENABLE_STATIC']=True
        tc.variables['LIBCXX_ENABLE_SHARED']=self.options.shared
        tc.variables['LIBCXX_ENABLE_STATIC_ABI_LIBRARY']=True
        #TODO consider using this ?
        #tc.variables['LIBCXX_STATICALLY_LINK_ABI_IN_STATIC_LIBRARY']=True
        tc.variables['LIBCXX_CXX_ABI']='libcxxabi'
        tc.variables["LIBCXX_INCLUDE_TESTS"] = False
        tc.variables["LIBCXX_LIBDIR_SUFFIX"] = ''
        tc.variables['LIBCXX_SOURCE_PATH']=source

        # libcxxabi paths
        # These are expected by LLVM's HandleLibCXXABI.cmake module
        include_paths = ";".join(self.dependencies["libcxxabi"].cpp_info.includedirs)
        lib_paths = ";".join(self.dependencies["libcxxabi"].cpp_info.libdirs)
        tc.variables['LIBCXX_CXX_ABI_INCLUDE_PATHS'] = include_paths
        tc.variables['LIBCXX_CXX_ABI_LIBRARY_PATH'] = lib_paths

        if (self.settings.compiler == "clang"):
            triple=self._triple_arch()+"-linux-elf"
            tc.variables["LIBCXX_TARGET_TRIPLE"] = triple
        tc.variables['LLVM_PATH']=llvm_source
        if (str(self.settings.arch) == "x86"):
            tc.variables['LLVM_BUILD_32_BITS']=True
        tc.generate()

    def build(self):

        if (self.settings.compiler == "gcc"):
            self.patch("libcxx",patch_file='files/float16_gcc.patch')
        cmake=CMake(self)
        source=self.source_folder+"/libcxx"        
        cmake.configure(build_script_folder=source)

        print("Building in {}".format(self.build_folder))
        print("Source in {}".format(self.source_folder))
        
        cmake.build(build_tool_args=['--trace'])

    def package(self):
        cmake=CMake(self)        
        cmake.install()
        source=self.source_folder+"/include"
        dest = self.package_folder+"/include"
        # TODO: figure out why this was needed👇. I don't think it has any effect.
        copy(self, pattern="*cxxabi*",dst=dest, src=source)
        

    def package_id(self):        
        self.info.settings.os = "ANY"

    def package_info(self):
        #this solves a lot but libcxx still needs to be included before musl
        self.cpp_info.includedirs = ['include','include/c++/v1']
        self.cpp_info.libs=['c++','c++experimental']
        self.cpp_info.libdirs=['lib']

    def deploy(self):
        self.copy("*",dst="include",src="include")
        self.copy("*.a",dst="lib",src="lib")

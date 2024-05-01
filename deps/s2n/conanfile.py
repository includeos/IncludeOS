import os
import shutil

from conan import ConanFile,tools
from conan.tools.scm import Git
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import copy

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

class S2nConan(ConanFile):
    settings="os","compiler","build_type","arch"
    name = "s2n"
    default_user = "includeos"
    version = "0.8"
    options = {
        "threads":[True, False]
    }
    default_options = {
        "threads": False
    }
    license = 'Apache 2.0'
    description = 's2n : an implementation of the TLS/SSL protocols'
    url = "https://www.openssl.org"

    def imports(self):
        self.copy("*",dst="target",src=".")

    def requirements(self):
        self.requires("openssl/1.1.1".format(self.user,self.channel))

    def source(self):
        repo = Git(self)
        args = ["--branch", self.version]
        repo.clone("https://github.com/fwsGonzo/s2n.git", target="s2n", args=args)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()        
        tc = CMakeToolchain(self)
        tc.variables["NO_STACK_PROTECTOR"]='ON'
        tc.variables["S2N_UNSAFE_FUZZING_MODE"]=False
        tc.variables["BUILD_TESTING"]='OFF'
        tc.variables["CMAKE_C_FLAGS"]="-Wno-error=strict-prototypes -fno-sanitize=undefined"
        
        print("Openssl:\n{} ".format(self.dependencies["openssl"].cpp_info.serialize()))

        # This property doesn't exist in conan2
        #tc.variables["CMAKE_PREFIX_PATH"]=self.dependencies["openssl"].cpp_info.rootpath
        
        tc.generate()
        
    def build(self):
        cmake = CMake(self)        
        cmake.configure(build_script_folder=self.source_folder+"/s2n")
        
        # Note: The tests won't build due to a linker error, missing symbols for
        #       some of the ubsan sanitizers. We shouldn't need the tests though, so
        #       building s2n specifically instad of all should be enough.        
        cmake.build(target="s2n")

    def package(self):
        cmake = CMake(self)
        cmake.install()        

    def package_info(self):
        self.cpp_info.libs=['s2n'] 


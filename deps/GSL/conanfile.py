import os
from conan import ConanFile,tools
from conan.tools.scm import Git
from conan.tools.files import copy
#from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout

#mark up GSL/Version@user/channel when building this one
#instead of only user/channel
#at the time of writing 1.0.0 and 2.0.0 are valid versions

class GslConan(ConanFile):
    name = "gsl"
    license = 'MIT'
    version = "2.0.0"
    description = 'C++ Guideline Support Library'
    url = "https://github.com/Microsoft/GSL"
    no_copy_source=True

    def source(self):
        print("DEBUG Source ")
        repo = Git(self)
        clone_args = ['--branch', "v{}".format(self.version)]
        repo.clone(url=self.url +".git", args=clone_args)

    def package(self):
        source = os.path.join(self.source_folder, "GSL", "include")
        dest   = os.path.join(self.package_folder, "include")
        print("DEBUG: src: {}\ndst: {}".format(source, dest))
        copy(self, pattern="*", src=source, dst=dest)

    def package_info(self):
        self.cpp_info.includedirs = ["include"]

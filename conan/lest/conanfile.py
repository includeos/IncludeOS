#README to build botan 2.8.0 use conan create (botan/2.8.0@user/channel) path to this file
import shutil

from conans import ConanFile,tools,CMake

class UplinkConan(ConanFile):
    #settings= "os","arch","build_type","compiler"
    name = "lest"
    version= "1.33.5"
    license = 'Apache-2.0'
    description = 'A modern,C++11-native, single-file header-only,tiny framework for unit-tests,TDD and BDD'
    url = "https://github.com/martinmoene/lest.git"

    def source(self):
        repo = tools.Git(folder="lest")
        repo.clone("https://github.com/martinmoene/lest.git",branch="v{}".format(self.version))

    def _cmake_configure(self):
        cmake = CMake(self)
        cmake.configure(source_folder=self.source_folder+"/lest")
        return cmake

    def build(self):
        cmake =self._cmake_configure()
        cmake.build()

    def package(self):
        cmake = self._cmake_configure()
        cmake.install()

    def deploy(self):
        self.copy("*",dst="include",src="include")
    

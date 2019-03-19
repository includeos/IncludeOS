import shutil
from conans import ConanFile,tools,CMake

class LibfdtConan(ConanFile):
    settings="os","compiler","build_type","arch"
    name = "libfdt"
    version = "1.4.7"
    license = 'BSD-2/GPL Dual licenced'
    description = 'Devicetree library'
    exports_sources='CMakeLists.txt'

    url = "https://github.com/dgibson/dtc.git"
    no_copy_source=True

    def source(self):
        repo = tools.Git(folder="dtc")
        repo.clone(self.url,branch="v{}".format(self.version))

    def configure(self):
        #doesnt matter what stdc++ lib you have this is C
        del self.settings.compiler.libcxx

    def _configure_cmake(self):
        cmake=CMake(self)
        cmake.configure(source_folder=self.source_folder)
        return cmake

    def build(self):
        cmake=self._configure_cmake()
        cmake.build()
        
    def package(self):
        cmake=self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs=['fdt']

    def deploy(self):
        self.copy("*.a",dst="lib",src="lib")
        self.copy("*.h",dst="include",src="include")

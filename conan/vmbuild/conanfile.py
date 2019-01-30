import os
from conans import ConanFile,tools,CMake

class VmbuildConan(ConanFile):
    settings= "os","arch"
    name = "vmbuild"
    license = 'Apache-2.0'
    description = 'Run your application with zero overhead'
    generators = 'cmake'
    url = "http://www.includeos.org/"

    def build_requirements(self):
        self.build_requires("GSL/2.0.0@includeos/test")

    def source(self):
        repo = tools.Git(folder="includeos")
        repo.clone("https://github.com/hioa-cs/IncludeOS.git",branch="conan")

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure(source_folder=self.source_folder+"/includeos/vmbuild")
        return cmake
    def build(self):
        cmake=self._configure_cmake()
        cmake.build()
    def package(self):
        cmake=self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.env_info.path.append(os.path.join(self.package_folder, "bin"))

    def deploy(self):
        self.copy("*",dst="bin",src="bin")

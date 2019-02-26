import shutil
from conans import ConanFile,tools,CMake

class DiscImagebuildConan(ConanFile):
    settings= "os","arch","build_type"
    name = "diskimagebuild"
    license = 'Apache-2.0'
    description = 'Run your application with zero overhead'
    generators = 'cmake'
    url = "http://www.includeos.org/"

    def source(self):
        repo = tools.Git(folder="includeos")
        repo.clone("https://github.com/hioa-cs/IncludeOS.git",branch="conan")

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure(source_folder=self.source_folder+"/includeos/diskimagebuild")
        return cmake
    def build(self):
        cmake=self._configure_cmake()
        cmake.build()
    def package(self):
        cmake=self._configure_cmake()
        cmake.install()

    def deploy(self):
        self.copy("*",dst="bin",src="bin")

#README to build botan 2.8.0 use conan create (botan/2.8.0@user/channel) path to this file
import shutil

from conans import ConanFile,tools,CMake

class LiveupdateConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "liveupdate"
    license = 'Apache-2.0'
    description = 'Run your application with zero overhead'
    generators = 'cmake'
    url = "http://www.includeos.org/"
    default_user="includeos"
    default_channel="test"

    def requirements(self):
        self.requires("s2n/1.1.1@{}/{}".format(self.user,self.channel))

    def build_requirements(self):
        self.build_requires("GSL/2.0.0@{}/{}".format(self.user,self.channel))

    def source(self):
        repo = tools.Git(folder="includeos")
        repo.clone("https://github.com/hioa-cs/IncludeOS.git",branch="conan")

    def _arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64"
        }.get(str(self.settings.arch))
    def _cmake_configure(self):
        cmake = CMake(self)
        cmake.definitions['ARCH']=self._arch()
        cmake.configure(source_folder=self.source_folder+"/includeos/lib/LiveUpdate")
        return cmake

    def build(self):
        cmake = self._cmake_configure()
        cmake.build()

    def package(self):
        cmake = self._cmake_configure()
        cmake.install()

    def package_info(self):
        #todo fix these in CMakelists.txt
        self.cpp_info.libs=['liveupdate']

    def deploy(self):
        #the first is for the editable version
        self.copy("*.a",dst="lib",src="build/lib")
        self.copy("liveupdate",dst="include",src=".")
        self.copy("*.hpp",dst="include",src=".")

        #TODO fix this in CMakelists.txt
        self.copy("*.a",dst="lib",src="lib")
        self.copy("*",dst="include",src="include")

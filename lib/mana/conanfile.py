#README to build botan 2.8.0 use conan create (botan/2.8.0@user/channel) path to this file
import shutil

from conans import ConanFile,tools,CMake

class ManaConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "mana"
    license = 'Apache-2.0'
    description = 'Run your application with zero overhead'
    generators = 'cmake'
    url = "http://www.includeos.org/"
    default_user="includeos"
    default_channel="test"

    def build_requirements(self):
        #TODO at some point put includeos as a dep for mana
        #removing everything below
        self.build_requires("libcxx/7.0.1@{}/{}".format(self.user,self.channel))
        self.build_requires("rapidjson/1.1.0@{}/{}".format(self.user,self.channel))
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
        cmake.configure(source_folder=self.source_folder+"/includeos/lib/mana")
        return cmake

    def build(self):
        cmake = self._cmake_configure()
        cmake.build()

    def package(self):
        cmake = self._cmake_configure()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs=['mana']

    def deploy(self):
        #the first is for the editable version
        self.copy("*.a",dst="lib",src="build/lib")
        #TODO fix this in mana cmake..
        self.copy("*.a",dst="lib",src="lib")
        self.copy("*",dst="include",src="include")

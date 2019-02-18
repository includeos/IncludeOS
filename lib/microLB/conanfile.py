#README to build botan 2.8.0 use conan create (botan/2.8.0@user/channel) path to this file
import shutil

from conans import ConanFile,tools,CMake

class MicroLBConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "microlb"
    license = 'Apache-2.0'
    description = 'Run your application with zero overhead'
    generators = 'cmake'
    url = "http://www.includeos.org/"
    version='0.13.0'
    
    default_user="includeos"
    default_channel="test"

    options={
        "liveupdate":[True,False],
        "tls": [True,False]
    }
    default_options={
        "liveupdate":True,
        "tls":True
    }
    def requirements(self):
        if (self.options.liveupdate):
            self.requires("liveupdate/{}@{}/{}".format(self.version,self.user,self.channel))
        if (self.options.tls):
            #this will put a dependency requirement on openssl
            self.requires("s2n/1.1.1@{}/{}".format(self.user,self.channel))

    def build_requirements(self):
        #these are header only so we dont need them down the value chain
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
        cmake.definitions['LIVEUPDATE']=self.options.liveupdate
        cmake.definitions['TLS']=self.options.tls
        cmake.configure(source_folder=self.source_folder+"/includeos/lib/microLB")
        return cmake

    def build(self):
        cmake = self._cmake_configure()
        cmake.build()

    def package(self):
        cmake = self._cmake_configure()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs=['microlb']

    def deploy(self):
        #for editable we need the 2 first
        self.copy("*.a",dst="lib",src="build/lib")
        self.copy("*.hpp",dst="include",src="micro_lb")

        self.copy("*.a",dst="lib",src="lib")
        self.copy("*",dst="include",src="include")

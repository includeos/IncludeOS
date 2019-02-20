#README to build botan 2.8.0 use conan create (botan/2.8.0@user/channel) path to this file
import shutil

from conans import ConanFile,tools,CMake

class ChainloaderConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "chainloader"
    license = 'Apache-2.0'
    description = 'IncludeOS 32->64 bit chainloader for x86'
    generators = 'cmake'
    url = "http://www.includeos.org/"

    default_options={
        "includeos:solo5":"OFF",
        "includeos:apple":'',
        "includeos:basic":"ON"
    }
    no_copy_source=True
    default_user="includeos"
    default_channel="test"

    def configure(self):
        if (self.settings.arch != "x86"):
            raise Exception(
            "Chainloader is only for x86 target architecture "
            "not: {}".format(self.settings.arch))
        del self.settings.compiler.libcxx
        #self.
    
    def build_requirements(self):
        self.build_requires("includeos/{}@{}/{}".format(self.version,self.user,self.channel))
        self.build_requires("libgcc/1.0@includeos/test")
        self.build_requires("vmbuild/{}@{}/{}".format(self.version,self.user,self.channel))
    
    def source(self):
        #shutil.copytree("/home/kristian/git/IncludeOS","includeos")
        repo = tools.Git(folder="includeos")
        repo.clone("https://github.com/hioa-cs/IncludeOS.git",branch="conan")

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions['INCLUDEOS_PREFIX']=self.build_folder
        cmake.configure(source_folder=self.source_folder+"/includeos/src/chainload")
        return cmake;

    def build(self):
        cmake=self._configure_cmake()
        cmake.build()


    def package_info(self):
        if self.settings.arch in ["x86","x86_64"]:
            self.settings.arch="x86_64"
    def package(self):
        cmake=self._configure_cmake()
        cmake.install()

    def deploy(self):
        #for editable packages
        self.copy("chainloader",dst="bin",src="build")
        self.copy("chainloader",dst="bin",src="bin")

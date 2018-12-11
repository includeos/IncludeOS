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
    #keep_imports=True
    def configure(self):
        if (self.settings.arch != "x86"):
            raise Exception(
            "Chainloader is only for x86 target architecture "
            "not: {}".format(self.settings.arch))
        del self.settings.compiler.libcxx
        #self.
    #hmm inst this a build requirements
    #def requirements(self):
    def build_requirements(self):
        self.build_requires("includeos/{}@{}/{}".format(self.version,self.user,self.channel))
        self.build_requires("libgcc/1.0@includeos/test")
        self.build_requires("vmbuild/{}@{}/{}".format(self.version,self.user,self.channel))
        #self.requires/"botan"
        #self.requires("vmbuild/0.13.0")
        #    def imports(self): #deploys everything to local directory..
        #        self.copy("*")
    #def build_requirements(self):
    def source(self):
        #repo = tools.Git(folder="includeos")
        #repo.clone("https://github.com/hioa-cs/IncludeOS.git",branch="conan")
        shutil.copytree("/home/kristian/git/IncludeOS","includeos")
    def _configure_cmake(self):
        cmake = CMake(self)
        #glad True and False also goes but not recursily
        #hmm do i need this in env..
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
        #self.copy("chainloader",dst="bin")
    #    cmake=self._configure_cmake()
        #we are doing something wrong this "shouldnt" trigger a new build
    #    cmake.install()
        #at this point we can copy things implace..
        #or we can use the "building with conan flag to deply things
        #in the right place"

        #arch include and arch lib is causing issues

    def deploy(self):
        self.copy("chainloader",dst="bin",src="bin")

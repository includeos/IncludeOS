import shutil
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

from conans import ConanFile,CMake,tools

class S2nConan(ConanFile):
    settings="os","compiler","build_type","arch"
    name = "s2n"
    version = "1.1.1" ##if we remove this line we can specify it from outside this script!! ps ps
    options = {"threads":[True, False]}
    #tag="OpenSSL_"+version.replace('.','_')
    default_options = {"threads": False}
    #options = {"shared":False}
    #branch = "version"+version
    license = 'Apache 2.0'
    description = 's2n : an implementation of the TLS/SSL protocols'
    url = "https://www.openssl.org"
    #exports_sources=['protobuf-options.cmake']
    #keep_imports=True
    #TODO handle build_requrements
    #def build_requirements(self):
        #self.build_requires("binutils/2.31@includeos/stable")
        #self.build_requires("musl/v1.1.18@includeos/stable")
        #self.build_requires("llvm/5.0@includeos/stable")## do we need this or just headers
    def configure(self):
        #TODO fix the FORTIFY_SOURCE ISSUE IN RELEASE
        del self.settings.build_type
    def requirements(self):
        self.requires("openssl/1.1.1@{}/{}".format(self.user,self.channel))

    def imports(self):
        self.copy("*",dst="target",src=".")

    def requirements(self):
        self.requires("openssl/1.1.1@{}/{}".format(self.user,self.channel))


    def source(self):
        repo = tools.Git(folder="s2n")
        repo.clone("https://github.com/fwsGonzo/s2n.git")

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["NO_STACK_PROTECTOR"]='ON'
        cmake.definitions["S2N_UNSAFE_FUZZING_MODE"]=''
        cmake.configure(source_folder="s2n")
        return cmake

    def build(self):
        cmake=self._configure_cmake()
        cmake.build()

    def package(self):
        cmake=self._configure_cmake()
        cmake.install()

    def deploy(self):
        self.copy("*.a",dst="lib",src="lib")
        self.copy("*.h",dst="include",src="include")

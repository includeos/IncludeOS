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
    options = {
        "threads":[True, False]
    }
    default_options = {
        "threads": False
    }
    license = 'Apache 2.0'
    description = 's2n : an implementation of the TLS/SSL protocols'
    url = "https://www.openssl.org"
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
        cmake.definitions["S2N_UNSAFE_FUZZING_MODE"]=False
        cmake.definitions["CMAKE_PREFIX_PATH"]=self.deps_cpp_info["openssl"].rootpath
        cmake.configure(source_folder="s2n")
        return cmake

    def build(self):
        cmake=self._configure_cmake()
        cmake.build()

    def package(self):
        cmake=self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs=['s2n']

    def deploy(self):
        self.copy("*.a",dst="lib",src="lib")
        self.copy("*.h",dst="include",src="include")

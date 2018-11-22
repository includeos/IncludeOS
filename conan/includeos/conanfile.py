#README to build botan 2.8.0 use conan create (botan/2.8.0@user/channel) path to this file
import shutil

from conans import ConanFile,tools,CMake

class IncludeOSConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "includeos"
    license = 'Apache-2.0'
    description = 'Run your application with zero overhead'
    generators = 'cmake'
    url = "http://www.includeos.org/"

    options = {
        "apple":['',True],
        "solo5":['ON','OFF'],
        "basic":['ON','OFF']
    }
    #actually we cant build without solo5 ?
    default_options= {
        "apple": '',
        "solo5": 'OFF',
        "basic": 'OFF'
    }
    #no_copy_source=True
    #keep_imports=True
    def requirements(self):
        self.requires("libcxx/[>=5.0]@includeos/test")## do we need this or just headers
        self.requires("GSL/2.0.0@includeos/test")


        if self.options.basic == 'OFF':
            self.requires("rapidjson/1.1.0@includeos/test")
            self.requires("http-parser/2.8.1@includeos/test") #this one is almost free anyways
            self.requires("uzlib/v2.1.1@includeos/test")
            self.requires("protobuf/3.5.1.1@includeos/test")
            self.requires("botan/2.8.0@includeos/test")
            self.requires("openssl/1.1.1@includeos/test")
            self.requires("s2n/1.1.1@includeos/test")



        if (self.options.apple):
            self.requires("libgcc/1.0@includeos/stable")
        if (self.options.solo5):
            self.requires("solo5/0.4.1@includeos/test")

    def imports(self):
        self.copy("*")

    def source(self):
        repo = tools.Git(folder="includeos")
        repo.clone("https://github.com/hioa-cs/IncludeOS.git",branch="conan")

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure(source_folder=self.source_folder+"/IncludeOS")
        return cmake;
    def build(self):
        cmake=self._configure_cmake()
        cmake.build()

    def package(self):
        cmake=self._configure_cmake()
        cmake.install()

    def deploy(self):
        self.copy("*",dst=".",src=".")

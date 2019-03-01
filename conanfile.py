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
    no_copy_source=True
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

        #if (self.options.apple):
            self.requires("libgcc/1.0@includeos/test")
        if (self.options.solo5):
            self.requires("solo5/0.4.1@includeos/test")
    def configure(self):
        del self.settings.compiler.libcxx
    def imports(self):
        self.copy("*")

    def source(self):
        repo = tools.Git(folder="includeos")
        repo.clone("https://github.com/hioa-cs/IncludeOS.git",branch="conan")

    def _target_arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64"
        }.get(str(self.settings.arch))
    def _configure_cmake(self):
        cmake = CMake(self)
        #glad True and False also goes but not recursily
        if (str(self.settings.arch) == "x86"):
            cmake.definitions['ARCH']="i686"
        else:
            cmake.definitions['ARCH']=str(self.settings.arch)
        if (self.options.basic):
            cmake.definitions['CORE_OS']=True
        cmake.definitions['WITH_SOLO5']=self.options.solo5
        cmake.configure(source_folder=self.source_folder+"/includeos")
        return cmake;

    def build(self):
        cmake=self._configure_cmake()
        cmake.build()

    def package(self):
        cmake=self._configure_cmake()
        #we are doing something wrong this "shouldnt" trigger a new build
        cmake.install()

    def package_info(self):
        #this is messy but unless we rethink things its the way to go
        self.cpp_info.includedirs=['include/os']
        platform='platform'
        #TODO se if this holds up for other arch's
        if (self.settings.arch == "x86" or self.settings.arch == "x86_64"):
            if ( self.options.basic == 'ON'):
                platform='x86_nano'
            else:
                platform='x86_pc'
        #if (self.settings.solo5):
        #if solo5 set solo5 as platform
        self.cpp_info.libs=[platform,'os','arch','musl_syscalls']
        self.cpp_info.libdirs = [
            '{}/lib'.format(self._target_arch()),
            '{}/platform'.format(self._target_arch())
        ]
    def deploy(self):
        self.copy("*",dst=".",src=".")

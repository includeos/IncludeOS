import shutil

from conans import ConanFile, python_requires, CMake
from conans.errors import ConanInvalidConfiguration

conan_tools = python_requires("conan-tools/[>=1.0.0]@includeos/stable")

class IncludeOSConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "includeos"
    version = conan_tools.git_get_semver()
    license = 'Apache-2.0'
    description = 'Run your application with zero overhead'
    generators = [ 'cmake','virtualenv' ]
    url = "http://www.includeos.org/"
    scm = {
        "type": "git",
        "url": "auto",
        "subfolder": ".",
        "revision": "auto"
    }

    options = {
        'platform': [
            'default',
            'nano',
            'solo5-hvt',
            'solo5-spt',
            'userspace'
        ],
        'smp': [ True, False ]
    }

    default_options = {
        'platform':'default',
        'smp': False
    }

    no_copy_source=True
    def requirements(self):
        self.requires("libcxx/[>=5.0]@includeos/stable")## do we need this or just headers
        self.requires("GSL/2.0.0@includeos/stable")
        self.requires("libgcc/1.0@includeos/stable")

        if self.settings.arch == "armv8":
            self.requires("libfdt/1.4.7@includeos/stable")

        if not self.options.platform == 'nano':
            self.requires("rapidjson/1.1.0@includeos/stable")
            self.requires("http-parser/2.8.1@includeos/stable") #this one is almost free anyways
            self.requires("uzlib/v2.1.1@includeos/stable")
            self.requires("botan/2.8.0@includeos/stable")
            self.requires("openssl/1.1.1@includeos/stable")
            self.requires("s2n/0.8@includeos/stable")

        if self.options.platform == 'solo5-hvt' or self.options.platform == 'solo5-spt':
            if self.settings.compiler != 'gcc' or self.settings.arch == "x86":
                raise ConanInvalidConfiguration("solo5 is only supported with gcc")
            self.requires("solo5/0.4.1@includeos/stable")

    def configure(self):
        if self.options.platform == 'solo5-hvt':
            self.options["solo5"].tenders='hvt'
        if self.options.platform == 'solo5-spt':
            self.options["solo5"].tenders='spt'

        del self.settings.compiler.libcxx

    def _target_arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64",
            "armv8" : "aarch64"
        }.get(str(self.settings.arch))

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions['VERSION']=self.version
        cmake.definitions['PLATFORM']=self.options.platform
        cmake.definitions['SMP']=self.options.smp
        cmake.configure(source_folder=self.source_folder)
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
        self.cpp_info.resdirs=[self.package_folder]

        # this puts os.cmake in the path
        self.cpp_info.builddirs = ["cmake"]

        # this ensures that API is searchable
        self.cpp_info.includedirs=['include/os']

        platform = {
            'default' : '{}_pc'.format(self._target_arch()),
            'nano' : '{}_nano'.format(self._target_arch()),
            'solo5-hvt' : '{}_solo5-hvt'.format(self._target_arch()),
            'solo5-spt' : '{}_solo5-spt'.format(self._target_arch()),
            'userspace' : '{}_userspace'.format(self._target_arch())
        }

        self.cpp_info.libs=['os','arch','musl_syscalls']
        self.cpp_info.libs.append(platform.get(str(self.options.platform),"NONE"))
        self.cpp_info.libdirs = [ 'lib', 'platform' ]

    def deploy(self):
        self.copy("*",dst="cmake",src="cmake")
        self.copy("*",dst="lib",src="lib")
        self.copy("*",dst="drivers",src="drivers")
        self.copy("*",dst="plugins",src="plugins")
        self.copy("*",dst="os",src="os")

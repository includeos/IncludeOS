import shutil

from conans import ConanFile,tools,CMake

def get_version():
    git = tools.Git()
    try:
        prev_tag = git.run("describe --tags --abbrev=0")
        commits_behind = int(git.run("rev-list --count %s..HEAD" % (prev_tag)))
        # Commented out checksum due to a potential bug when downloading from bintray
        #checksum = git.run("rev-parse --short HEAD")
        if prev_tag.startswith("v"):
            prev_tag = prev_tag[1:]
        if commits_behind > 0:
            prev_tag_split = prev_tag.split(".")
            prev_tag_split[-1] = str(int(prev_tag_split[-1]) + 1)
            output = "%s-%d" % (".".join(prev_tag_split), commits_behind)
        else:
            output = "%s" % (prev_tag)
        return output
    except:
        return None

class IncludeOSConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "includeos"
    version = get_version()
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
    default_user='includeos'
    default_channel='test'
    #keep_imports=True
    def requirements(self):
        self.requires("libcxx/[>=5.0]@{}/{}".format(self.user,self.channel))## do we need this or just headers
        self.requires("GSL/2.0.0@{}/{}".format(self.user,self.channel))
        self.requires("libgcc/1.0@{}/{}".format(self.user,self.channel))

        if self.settings.arch == "armv8":
            self.requires("libfdt/1.4.7@includeos/test")

        if self.options.basic == 'OFF':
            self.requires("rapidjson/1.1.0@{}/{}".format(self.user,self.channel))
            self.requires("http-parser/2.8.1@{}/{}".format(self.user,self.channel)) #this one is almost free anyways
            self.requires("uzlib/v2.1.1@{}/{}".format(self.user,self.channel))
            self.requires("botan/2.8.0@{}/{}".format(self.user,self.channel))
            self.requires("openssl/1.1.1@{}/{}".format(self.user,self.channel))
            self.requires("s2n/1.1.1@{}/{}".format(self.user,self.channel))

        if (self.options.solo5):
            self.requires("solo5/0.4.1@{}/{}".format(self.user,self.channel))

    def configure(self):
        del self.settings.compiler.libcxx

    def _target_arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64",
            "armv8" : "aarch64"
        }.get(str(self.settings.arch))
    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions['ARCH']=self._target_arch()
        if (self.options.basic):
            cmake.definitions['CORE_OS']=True
        cmake.definitions['WITH_SOLO5']=self.options.solo5
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
        platform='platform'
        #TODO se if this holds up for other arch's
        if (self.settings.arch == "x86" or self.settings.arch == "x86_64"):
            if ( self.options.basic == 'ON'):
                platform='x86_nano'
            else:
                platform='x86_pc'
        if (self.settings.arch == "armv8"):
            platform='aarch64_vm'
        #if (self.settings.solo5):
        #if solo5 set solo5 as platform
        self.cpp_info.libs=[platform,'os','arch','musl_syscalls']
        self.cpp_info.libdirs = [
            'lib',
            'platform'
        ]

    def deploy(self):
        self.copy("*",dst="cmake",src="cmake")
        self.copy("*",dst="lib",src="lib")
        self.copy("*",dst="drivers",src="drivers")
        self.copy("*",dst="plugins",src="plugins")
        self.copy("*",dst="os",src="os")

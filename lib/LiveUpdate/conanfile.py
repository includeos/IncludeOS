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
        return '0.0.0'


class LiveupdateConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "liveupdate"
    license = 'Apache-2.0'
    version = get_version()
    description = 'Run your application with zero overhead'
    generators = 'cmake'
    url = "http://www.includeos.org/"

    default_user="includeos"
    default_channel="test"

    scm = {
        "type" : "git",
        "url" : "auto",
        "subfolder": ".",
        "revision" : "auto"
    }
    def requirements(self):
        self.requires("includeos/[>=0.14.0,include_prerelease=True]@{}/{}".format(self.user,self.channel))
        self.requires("s2n/1.1.1@{}/{}".format(self.user,self.channel))

    def build_requirements(self):
        self.build_requires("GSL/2.0.0@{}/{}".format(self.user,self.channel))

    def _arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64",
            "armv8" : "aarch64"
        }.get(str(self.settings.arch))

    def _cmake_configure(self):
        cmake = CMake(self)
        cmake.definitions['ARCH']=self._arch()
        cmake.configure(source_folder=self.source_folder+"/lib/LiveUpdate")
        return cmake

    def build(self):
        cmake = self._cmake_configure()
        cmake.build()

    def package(self):
        cmake = self._cmake_configure()
        cmake.install()

    def package_info(self):
        #todo fix these in CMakelists.txt
        self.cpp_info.libs=['liveupdate']

    def deploy(self):
        #the first is for the editable version
        self.copy("*.a",dst="lib",src="build/lib")
        self.copy("liveupdate",dst="include",src=".")
        self.copy("*.hpp",dst="include",src=".")

        #TODO fix this in CMakelists.txt
        self.copy("*.a",dst="lib",src="lib")
        self.copy("*",dst="include",src="include")

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


class ChainloaderConan(ConanFile):
    name = "chainloader"
    version=get_version()
    license = 'Apache-2.0'
    description = 'IncludeOS 32->64 bit chainloader for x86'
    generators = ['cmake','virtualenv']
    url = "http://www.includeos.org/"
    scm = {
        "type": "git",
        "url": "auto",
        "subfolder": ".",
        "revision": "auto"
    }

    default_options={
        "includeos:solo5":"OFF",
        "includeos:apple":'',
        "includeos:basic":"ON"
    }
    no_copy_source=True

    default_user="includeos"
    default_channel="test"

    #def requirements(self):
    #    self.requires("includeos/[>=0.14.0,include_prerelease=True]@{}/{}".format(self.user,self.channel),private=True)

    def build_requirements(self):
        self.build_requires("vmbuild/[>=0.14.0,include_prerelease=True]@{}/{}".format(self.user,self.channel))
        self.build_requires("includeos/[>=0.14.0,include_prerelease=True]@{}/{}".format(self.user,self.channel))

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure(source_folder=self.source_folder+"/src/chainload")
        return cmake

    def build(self):
        cmake=self._configure_cmake()
        cmake.build()

    def package_info(self):
        self.env_info.INCLUDEOS_CHAINLOADER=self.package_folder+"/bin"

    def package(self):
        cmake=self._configure_cmake()
        cmake.install()

    def deploy(self):
        self.copy("chainloader",dst="bin",src="bin")

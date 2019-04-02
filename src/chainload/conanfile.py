from conans import ConanFile, python_requires, CMake

conan_tools = python_requires("conan-tools/[>=1.0.0]@includeos/stable")

class ChainloaderConan(ConanFile):
    name = "chainloader"
    version = conan_tools.git_get_semver()
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
        "includeos:platform":"nano"
    }
    no_copy_source=True

    default_user="includeos"
    default_channel="latest"


    def package_id(self):
        self.info.requires.major_mode()

    def build_requirements(self):
        self.build_requires("includeos/[>=0.14.0,include_prerelease=True]@{}/{}".format(self.user,self.channel))
        self.build_requires("vmbuild/0.15.0@includeos/stable")

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

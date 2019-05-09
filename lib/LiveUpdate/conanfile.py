from conans import ConanFile, python_requires, CMake

conan_tools = python_requires("conan-tools/[>=1.0.0]@includeos/stable")

class LiveupdateConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "liveupdate"
    license = 'Apache-2.0'
    version = conan_tools.git_get_semver()
    description = 'Run your application with zero overhead'
    generators = 'cmake'
    url = "http://www.includeos.org/"

    default_user="includeos"
    default_channel="latest"

    scm = {
        "type" : "git",
        "url" : "auto",
        "subfolder": ".",
        "revision" : "auto"
    }

    def package_id(self):
        self.info.requires.major_mode()

    def requirements(self):
        self.requires("includeos/[>=0.14.0,include_prerelease=True]@{}/{}".format(self.user,self.channel))
        self.requires("s2n/0.8@includeos/stable")

    def build_requirements(self):
        self.build_requires("GSL/2.0.0@includeos/stable")

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

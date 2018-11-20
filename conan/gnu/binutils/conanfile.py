import os
from conans import python_requires,AutoToolsBuildEnvironment
base = python_requires("GnuBase/0.1/includeos/gnu")

class BinutilsConan(base.GnuConan):
    settings= "compiler","arch","os"
    name = "binutils"
    url = "https://www.gnu.org/software/binutils/"
    description = "The GNU Binutils are a collection of binary tools."
    compression='xz'

    def build(self):
        env_build = AutoToolsBuildEnvironment(self)
        env_build.configure(configure_dir="binutils-%s"%self.version,target=str(self.settings.arch)+"-elf",args=["--disable-nls","--disable-werror"]) #what goes in here preferably
        env_build.make()
        env_build.install()

    def package(self):
        self.copy("*",dst=str(self.settings.arch)+"-elf",src=str(self.settings.arch)+'-elf')
        self.copy("*.h", dst="include", src="binutils/include")
        self.copy("*.a", dst="lib", keep_path=False)
        self.copy("*",dst="bin",src="bin")

    def package_info(self):
        self.env_info.path.append(os.path.join(self.package_folder, "bin"))

    def deploy(self):
        self.copy("*",dst="bin",src="bin")
        self.copy("*",dst="include",src="include")
        self.copy("*",src="lib",dst="lib")
        self.copy("*",dst=str(self.settings.arch)+"-elf",src=str(self.settings.arch)+'-elf')

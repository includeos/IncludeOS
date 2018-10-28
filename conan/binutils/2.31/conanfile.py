#binutils recepie first take!!
#todo figure out to get a build directory ?
#todo use shutil to move versioned to unversioned ?

import os
from conans import ConanFile,tools,AutoToolsBuildEnvironment

class BinutilsConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "binutils"
    version = "2.31"
    url = "TODO: https://ftp.gnu.org/gnu/binutils"
    def source(self):
        zip_name="binutils-%s.tar.gz"%self.version
        tools.download("https://ftp.gnu.org/gnu/binutils/%s" % zip_name,zip_name)
        tools.unzip(zip_name)
        os.unlink(zip_name)

    def build(self):
        env_build = AutoToolsBuildEnvironment(self)
        env_build.configure(configure_dir="binutils-%s"%self.version,target=str(self.settings.arch)+"-elf",args=["--disable-nls","--disable-werror"]) #what goes in here preferably
        env_build.make()
        env_build.install()

    def package(self):
        self.copy("*.h", dst="include", src="include")
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.env_info.path.append(os.path.join(self.package_folder, "bin"))

    def deploy(self):
        self.copy("*",dst="bin",src="bin")
        self.copy("*",dst=str(self.settings.arch)+"-elf",src=str(self.settings.arch)+"-elf")

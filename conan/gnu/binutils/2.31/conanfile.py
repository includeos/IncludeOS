import os
from conans import ConanFile,tools,AutoToolsBuildEnvironment

class BinutilsConan(ConanFile):
    #we dont care how you compiled it but which os and arch it is meant to run on and which arch its targeting
    #pre conan 2.0 we have to use arch_build as host arch and arch as target arch
    settings= "arch_build","os_build","arch"
    name = "binutils"
    version = "2.31"
    url = "https://www.gnu.org/software/binutils/"
    description = "The GNU Binutils are a collection of binary tools."
    license = "GNU GPL"
    def source(self):
        zip_name="binutils-{}.tar.gz".format(self.version)
        tools.download("https://ftp.gnu.org/gnu/binutils/%s" % zip_name,zip_name)
        tools.unzip(zip_name)

    def _find_arch(self):
        if str(self.settings.arch) == "x86":
            return "i386"
        return str(self.settings.arch)

    def _find_host_arch(self):
        if str(self.settings.arch_build) == "x86":
            return "i386"
        return str(self.settings.arch_build)

    def build(self):
        arch=self._find_arch()
        env_build = AutoToolsBuildEnvironment(self)
        env_build.configure(configure_dir="binutils-{}".format(self.version),
            target=arch+"-elf",
            host=self._find_host_arch()+"-pc-linux-gnu",
            args=["--disable-nls","--disable-werror"]) #what goes in here preferably
        env_build.make()
        env_build.install()


    def package(self):
        arch=self._find_arch()
        self.copy("*",dst=arch+"-elf",src=arch+'elf')
        self.copy("*.h", dst="include", src="include")
        self.copy("*.a", dst="lib", keep_path=False)
        self.copy("*",dst="bin",src="bin")

    def package_info(self):
        self.info.settings.arch_build="ANY"
        self.env_info.path.append(os.path.join(self.package_folder, "bin"))

    def deploy(self):
        arch=self._find_arch()
        self.copy("*",dst=arch+"-elf",src=arch+'elf')
        self.copy("*",dst="bin",src="bin")
        self.copy("*",dst="include",src="include")

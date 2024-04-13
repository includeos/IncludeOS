import os
from conans import ConanFile,tools

class Solo5Conan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "solo5"
    version = "0.4.1"
    url = "https://github.com/includeos/solo5.git"
    description = "A sandboxed execution environment for unikernels. Linux only for now."
    license = "ISC"
    options = {
        "tenders":['hvt','spt']
    }
    default_options = {
        "tenders":'hvt'
    }

    def source(self):
        repo = tools.Git(folder = self.name)
        repo.clone(self.url, branch="ssp-fix")

    def build(self):
        self.run("CC=gcc ./configure.sh", cwd=self.name)
        self.run("make", cwd=self.name)
        self.run("ar rcs libsolo5_hvt.a bindings/hvt/solo5_hvt.o",cwd="solo5")
        self.run("ar rcs libsolo5_spt.a bindings/spt/solo5_spt.o",cwd="solo5")

    def package(self):
        #grab evenrything just so its a reausable redistributable recipe
        self.copy("*.h", dst="include/solo5", src=self.name + "/include/solo5")
        self.copy("*.a", dst="lib", src=self.name)
        self.copy("solo5-hvt", dst="bin", src= self.name + "/tenders/hvt")
        self.copy("solo5-hvt-configure", dst="bin", src= self.name + "/tenders/hvt")
        self.copy("solo5-spt", dst="bin", src= self.name + "/tenders/spt")

    def package_info(self):
        if self.options.tenders == 'hvt':
            self.cpp_info.libs=['solo5_hvt']
        if self.options.tenders == 'spt':
            self.cpp_info.libs=['solo5_spt']

        self.env_info.path.append(os.path.join(self.package_folder,"bin"))

    def deploy(self):
        self.copy("*", dst="lib",src="lib")
        self.copy("*", dst="bin",src="bin")
        self.copy("*", dst="include", src="include")

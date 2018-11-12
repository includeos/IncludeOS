import os
from conans import ConanFile,tools

class Solo5Conan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "solo5"
    version = "0.3.1"
    url = "https://github.com/Solo5/solo5"
    description = "A sandboxed execution environment for unikernels. Linux only for now."
    license = "ISC"

    def source(self):
        repo = tools.Git(folder = self.name)
        repo.clone(self.url + ".git")
        repo.checkout("v" + self.version)

    def build(self):
        self.run("CC=gcc ./configure.sh", cwd=self.name)
        self.run("make", cwd=self.name)

    def package(self):
        self.copy("solo5.h", dst="include", src=self.name + "/kernel/")
        self.copy("solo5.o", dst="lib", src=self.name + "/kernel/ukvm/")
        self.copy("ukvm-bin", dst="lib", src= self.name + "/ukvm/")

    def deploy(self):
        self.copy("*", dst="lib",src="lib")
        self.copy("*", dst="include", src="include")

import os
from conan import ConanFile,tools
from conan.tools.scm import Git
from conan.tools.files import copy

class BotanConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "botan"
    default_user = "includeos"
    version = "3.4.0" # 👈 NOTE: This version is not yet tested with IncludeOS. It compiles.
    license = 'BSD 2-Clause'
    description = 'Botan: Crypto and TLS for Modern C++'
    url = "https://github.com/Tencent/rapidjson/"

    @property
    def default_channel(self):
        return "stable"

    keep_imports=True

    def requirements(self):
        self.requires("libcxx/[>=5.0]".format(self.user,self.channel))
        self.requires("musl/[>=1.1.18]".format(self.user,self.channel))
        

    def source(self):
        repo = Git(self)
        args=["--branch", str(self.version)]
        repo.clone("https://github.com/randombit/botan.git", args = args, target="botan")

    def build(self):
        #TODO at some point fix the msse3
        env_inc="  -I"+self.build_folder+"/include/c++/v1 -I"+self.build_folder+"/include -Ibuild/include/botan"
        cmd="./configure.py --os=linux --disable-shared --cpu="+str(self.settings.arch)
        if self.settings.compiler == "gcc":
            if self.settings.arch == "x86_64":
                target="-m64"
            if self.settings.arch == "x86":
                target="-m32"
        if self.settings.compiler == "clang":
            target="--target="+str(self.settings.arch)+"-pc-linux-gnu"
        flags="\" "+target+" -msse3 -D_GNU_SOURCE"+env_inc+"\""
        cmd = cmd + " --cc-abi-flags="+flags
        print("Building in {}. Comamnd: \n{}".format(self.build_folder, cmd))
        self.run(cmd,cwd="botan")
        self.run("make -j$(nproc) libs",cwd="botan")

    def package(self):
        src_inc = os.path.join(self.source_folder, "botan", "build", "include", "botan")
        dst_inc = os.path.join(self.package_folder, "include", "botan")
        src_lib = os.path.join(self.source_folder, "botan")
        dst_lib = os.path.join(self.package_folder, "lib")
        copy(self, pattern="*.h", dst=dst_inc, src=src_inc)
        copy(self, pattern="*.a", dst=dst_lib, src=src_lib)


#README to build botan 2.8.0 use conan create (botan/2.8.0@user/channel) path to this file

from conans import ConanFile,tools

class BotanConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "botan"
    license = 'BSD 2-Clause'
    description = 'Botan: Crypto and TLS for Modern C++'
    url = "https://github.com/Tencent/rapidjson/"

    keep_imports=True
    def build_requirements(self):
        self.build_requires("binutils/2.31@includeos/stable")
        self.build_requires("musl/v1.1.18@includeos/stable")
        self.build_requires("llvm/5.0@includeos/stable")## do we need this or just headers

    def imports(self):
        self.copy("*",dst="target",src=".")

    def source(self):
        repo = tools.Git(folder="botan")
        repo.clone("https://github.com/randombit/botan.git")
        self.run("git fetch --all --tags --prune",cwd="botan")
        self.run("git checkout tags/"+str(self.version)+" -b "+str(self.version),cwd="botan")

    def build(self):
        #TODO at some point fix the msse3
        env_inc="  -I"+self.build_folder+"/target/libcxx/include -I"+self.build_folder+"/target/include -Ibuild/include/botan"
        cmd="./configure.py --os=includeos --disable-shared --cpu="+str(self.settings.arch)
        flags="\"--target="+str(self.settings.arch)+"-pc-linux-gnu -msse3 -D_LIBCPP_HAS_MUSL_LIBC -D_GNU_SOURCE -nostdlib -nostdinc++ "+env_inc+"\""
        self.run(cmd+" --cc-abi-flags="+flags,cwd="botan")
        self.run("make -j12 libs",cwd="botan")

    def package(self):
        self.copy("*.h",dst="include/botan",src="botan/build/include/botan")
        self.copy("*.a",dst="lib",src="botan")

    def deploy(self):
        self.copy("*.h",dst="include/botan",src="include/botan")
        self.copy("*.a",dst="lib",src="lib")

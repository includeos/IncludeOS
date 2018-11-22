from conans import ConanFile,tools

class BotanConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "botan"
    license = 'BSD 2-Clause'
    description = 'Botan: Crypto and TLS for Modern C++'
    url = "https://github.com/Tencent/rapidjson/"

    keep_imports=True

    def requirements(self):
        self.requires("libcxx/[>=5.0]@{}/{}".format(self.user,self.channel))
        self.requires("musl/v1.1.18@{}/{}".format(self.user,self.channel))
    def imports(self):
        self.copy("*",dst="include",src="include")

    def source(self):
        repo = tools.Git(folder="botan")
        repo.clone("https://github.com/randombit/botan.git",branch=str(self.version))

    def build(self):
        #TODO at some point fix the msse3
        env_inc="  -I"+self.build_folder+"/include/c++/v1 -I"+self.build_folder+"/include -Ibuild/include/botan"
        cmd="./configure.py --os=includeos --disable-shared --cpu="+str(self.settings.arch)
        if self.settings.compiler == "gcc":
            if self.settings.arch == "x86_64":
                target="-m64"
            if self.settings.arch == "x86":
                target="-m32"
        if self.settings.compiler == "clang":
            target="--target="+str(self.settings.arch)+"-pc-linux-gnu"
        flags="\" "+target+" -msse3 -D_GNU_SOURCE"+env_inc+"\""
        self.run(cmd+" --cc-abi-flags="+flags,cwd="botan")
        self.run("make -j12 libs",cwd="botan")

    def package(self):
        self.copy("*.h",dst="include/botan",src="botan/build/include/botan")
        self.copy("*.a",dst="lib",src="botan")

    def deploy(self):
        self.copy("*.h",dst="include/botan",src="include/botan")
        self.copy("*.a",dst="lib",src="lib")

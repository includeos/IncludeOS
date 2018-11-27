#README to build botan 2.8.0 use conan create (botan/2.8.0@user/channel) path to this file
import shutil

from conans import ConanFile,tools,CMake

class LiveupdateConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "liveupdate"
    license = 'Apache-2.0'
    description = 'Run your application with zero overhead'
    generators = 'cmake'
    url = "http://www.includeos.org/"

    def build_requirements(self):
        self.build_requires("GSL/2.0.0@{}/{}".format(self.user,self.channel))

    def source(self):
        #repo = tools.Git(folder="includeos")
        #repo.clone("https://github.com/hioa-cs/IncludeOS.git",branch="conan")
        shutil.copytree("/home/kristian/git/IncludeOS","IncludeOS")

    def _arch(self):
        return {
            "x86":"i686",
            "x86_64":"x86_64"
        }.get(str(self.settings.arch))
    def _cmake_configure(self):
        cmake = CMake(self)
        cmake.definitions['ARCH']=self._arch()
        cmake.configure(source_folder=self.source_folder+"/IncludeOS/lib/LiveUpdate")
        return cmake

    def build(self):
        cmake = self._cmake_configure()
        cmake.build()
        #print("TODO")
        #TODO at some point fix the msse3
        #env_inc="  -I"+self.build_folder+"/target/libcxx/include -I"+self.build_folder+"/target/include -Ibuild/include/botan"
        #cmd="./configure.py --os=includeos --disable-shared --cpu="+str(self.settings.arch)
        #flags="\"--target="+str(self.settings.arch)+"-pc-linux-gnu -msse3 -D_LIBCPP_HAS_MUSL_LIBC -D_GNU_SOURCE -nostdlib -nostdinc++ "+env_inc+"\""
        #self.run(cmd+" --cc-abi-flags="+flags,cwd="botan")
        #self.run("make -j12 libs",cwd="botan")
    def package(self):
        cmake = self._cmake_configure()
        cmake.install()

    def deploy(self):
        self.copy("*",dst="bin",src="bin")
        self.copy("*",dst="include",src="include")

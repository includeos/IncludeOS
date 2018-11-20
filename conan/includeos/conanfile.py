#README to build botan 2.8.0 use conan create (botan/2.8.0@user/channel) path to this file
import shutil

from conans import ConanFile,tools,CMake

class IncludeOSConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "includeos"
    license = 'Apache-2.0'
    description = 'Run your application with zero overhead'
    generators = 'cmake'
    url = "http://www.includeos.org/"

    options = {"apple":[True, False], "solo5":[True,False]}
    #actually we cant build without solo5 ?
    default_options = {"apple": False, "solo5" : True}
    #no_copy_source=True
    #keep_imports=True
    def build_requirements(self):
        self.build_requires("libcxxabi/[>=5.0]@includeos/test")## do we need this or just headers
        self.build_requires("libcxx/[>=5.0]@includeos/test")## do we need this or just headers
        self.build_requires("libunwind/[>=5.0]@includeos/test")## do we need this or just headers
        self.build_requires("musl/v1.1.18@includeos/test")
        self.build_requires("binutils/[>=2.31]@includeos/test")


        self.build_requires("GSL/1.0.0@includeos/test")
        self.build_requires("protobuf/3.5.1.1@includeos/test")
        self.build_requires("rapidjson/1.1.0@includeos/test")
        self.build_requires("botan/2.8.0@includeos/test")
        self.build_requires("openssl/1.1.1@includeos/test")
        self.build_requires("s2n/1.1.1@includeos/test")

        self.build_requires("http-parser/2.8.1@includeos/test")
        self.build_requires("uzlib/v2.1.1@includeos/test")

        if (self.options.apple):
            self.build_requires("libgcc/1.0@includeos/stable")
        if (self.options.solo5):
            self.build_requires("solo5/0.3.1@includeos/test")

    #this is a very raw way of doing this
    #def imports(self):
    # this is NASTY imho
    #    self.copy("*",dst=self.env.INCLUDEOS_PREFIX,src=".")

    def source(self):

        #repo = tools.Git(folder="includeos")
        #repo.clone("https://github.com/hioa-cs/IncludeOS.git",branch="conan")
        shutil.copytree("/home/kristian/git/IncludeOS","IncludeOS")

    def _configure_cmake(self):
        cmake = CMake(self)
        #cmake.definitions["CMAKE_C_FLAGS"] =''
        #cmake.definitions["CMAKE_CXX_FLAGS"]=''
        #cmake.definitions['CMAKE_INSTALL_PREFIX']=self.package_folder+"/include"
        cmake.configure(source_folder=self.source_folder+"/IncludeOS")
        #STOP conan from spamming but we should get flags from profile ?
        return cmake;
    def build(self):
        cmake=self._configure_cmake()
        cmake.build()

        #print("TODO")
        #TODO at some point fix the msse3
        #env_inc="  -I"+self.build_folder+"/target/libcxx/include -I"+self.build_folder+"/target/include -Ibuild/include/botan"
        #cmd="./configure.py --os=includeos --disable-shared --cpu="+str(self.settings.arch)
        #flags="\"--target="+str(self.settings.arch)+"-pc-linux-gnu -msse3 -D_LIBCPP_HAS_MUSL_LIBC -D_GNU_SOURCE -nostdlib -nostdinc++ "+env_inc+"\""
        #self.run(cmd+" --cc-abi-flags="+flags,cwd="botan")
        #self.run("make -j12 libs",cwd="botan")
    def package(self):
        cmake=self._configure_cmake()
        cmake.install()
    #def package(self):
        #i cant because i used cmake install .. crap..d
        #self.copy("*",dst="include/includeos",src="includeos")
    #    print("TODO?")
        #self.copy("*.h",dst="include/botan",src="botan/build/include/botan")
        #self.copy("*.a",dst="lib",src="botan")

    def deploy(self):
        self.copy("*",dst="bin",src="bin")
        self.copy("*",dst="includeos",src="includeos")
        #print("TODO")
        #self.copy("*.h",dst="include/botan",src="include/botan")
        #self.copy("*.a",dst="lib",src="lib")

import shutil
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

from conans import ConanFile,CMake,tools

class S2nConan(ConanFile):
    settings="os","compiler","build_type","arch"
    name = "s2n"
    version = "1.1.1" ##if we remove this line we can specify it from outside this script!! ps ps
    options = {"threads":[True, False]}
    #tag="OpenSSL_"+version.replace('.','_')
    default_options = {"threads": False}
    #options = {"shared":False}
    #branch = "version"+version
    license = 'Apache 2.0'
    description = 's2n : an implementation of the TLS/SSL protocols'
    url = "https://www.openssl.org"
    #exports_sources=['protobuf-options.cmake']
    #keep_imports=True
    #TODO handle build_requrements
    #def build_requirements(self):
        #self.build_requires("binutils/2.31@includeos/stable")
        #self.build_requires("musl/v1.1.18@includeos/stable")
        #self.build_requires("llvm/5.0@includeos/stable")## do we need this or just headers
    def build_requirements(self):
        self.build_requires("openssl/1.1.1@%s/%s"%(self.user,self.channel))

    def imports(self):
        self.copy("*",dst="target",src=".")

    def requirements(self):
        self.requires("openssl/1.1.1@%s/%s"%(self.user,self.channel))


    def source(self):
        repo = tools.Git(folder="s2n")
        repo.clone("https://github.com/fwsGonzo/s2n.git")
        #self.run("git fetch --all --tags --prune",cwd="openssl")
        #self.run("git checkout tags/"+str(self.tag)+" -b "+str(self.tag),cwd="openssl")

    def build(self):

        #cmake calls findpackage for libcrypto.a .. so we should provide that feature from openssl build ?
        cmake = CMake(self)
        cmake.definitions["NO_STACK_PROTECTOR"]='ON'
        cmake.definitions["S2N_UNSAFE_FUZZING_MODE"]=''

        cmake.configure(source_folder="s2n")
        cmake.build(target="s2n")
        #cmake.build(target='unwind')
        #TODO handle arch target and optimalizations
        #TODO use our own includes!
        #options=" no-shared no-ssl3 enable-ubsan "
        #if (not self.options.threads):
        #    options+=" no-threads "
        #if ()
        #self.run("./Configure --prefix="+self.package_folder+" --libdir=lib no-ssl3-method enable-ec_nistp_64_gcc_128 linux-x86_64 "+flags,cwd="openssl")
        #self.run(("./config --prefix="+self.package_folder+" --openssldir="+self.package_folder+options),cwd="openssl" )
        #self.run("make -j16 depend",cwd="openssl")
        #self.run("make -j16",cwd="openssl")


    def package(self):
        self.copy("*.h",dst="include",src="s2n/api")
        self.copy("*.a",dst="lib",src="lib")
        #print("TODO")
        #todo extract to includeos/include!!
        #self.copy("*",dst="include/rapidjson",src="rapidjson/include/rapidjson")

    def deploy(self):
        self.copy("*.a",dst="lib",src="lib")
        self.copy("*.h",dst="include",src="include")
        #self.copy("*.a",dst="lib",src="openssl")
        #print("TODO")
        #self.copy("*",dst="include/rapidjson",src="include/rapidjson")

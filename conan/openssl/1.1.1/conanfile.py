import shutil
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

from conans import ConanFile,CMake,tools

class OpenSSLConan(ConanFile):
    settings="os","compiler","build_type","arch"
    name = "openssl"
    version = "1.1.1" ##if we remove this line we can specify it from outside this script!! ps ps

    options = {
        "threads":[True, False],
        "shared":[True,False],
        "ubsan" : [True,False],
        "async" : [True,False]
        }

    default_options = {
        "threads": True,
        "shared": False,
        "ubsan" : False,
        "async" : False
        }

    license = 'Apache 2.0'
    description = 'A language-neutral, platform-neutral extensible mechanism for serializing structured data.'
    url = "https://www.openssl.org"

    def requirements(self):
        self.requires("libcxx/[>=5.0]@{}/{}".format(self.user,self.channel))

    def imports(self):
        self.copy("*",dst="include",src="include")

    def source(self):
        tag="OpenSSL_"+self.version.replace('.','_')
        repo = tools.Git(folder="openssl")
        repo.clone("https://github.com/openssl/openssl.git",branch=tag)

    def build(self):
        #TODO handle arch target and optimalizations
        #TODO use our own includes!
        #TODO TODO
        options=["no-ssl3"]
        if self.options.ubsan:
            options+=['enable-ubsan']
        if not self.options.threads:
            options+=['no-threads']
        if not self.options.shared:
            options+=['no-shared']
        if not self.options.async:
            options+=['no-async']
        if str(self.settings.arch) == "x86":
            options+=['386']

        options+=["-Iinclude/c++/v1","-Iinclude"]
        #self.run("./Configure --prefix="+self.package_folder+" --libdir=lib no-ssl3-method enable-ec_nistp_64_gcc_128 linux-x86_64 "+flags,cwd="openssl")
        self.run(("./config --prefix="+self.package_folder+" --openssldir="+self.package_folder+" ".join(options)),cwd="openssl" )
        self.run("make -j16 depend",cwd="openssl")
        self.run("make -j16",cwd="openssl")


    def package(self):
        self.copy("*.h",dst="include/openssl",src="openssl/include/openssl")
        self.copy("*.a",dst="lib",src="openssl")
        #print("TODO")
        #todo extract to includeos/include!!
        #self.copy("*",dst="include/rapidjson",src="rapidjson/include/rapidjson")
    def package_info(self):
        self.cpp_info.libs=['crypto','openssl']

    def deploy(self):
        self.copy("*.h",dst="include/openssl",src="openssl/include/openssl")
        self.copy("*.a",dst="lib",src="lib")
        #print("TODO")
        #self.copy("*",dst="include/rapidjson",src="include/rapidjson")

import os, shutil
from conan import ConanFile,tools
from conan.tools.scm import Git
from conan.tools.files import copy
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

# Note: Requires static libusan
# e.g. /usr/lib/llvm-18/lib/clang/18/lib/linux/libclang_rt.ubsan_standalone-x86_64.a
# you get that if you install llvm via their script from https://apt.llvm.org/

class OpenSSLConan(ConanFile):
    settings="os","compiler","build_type","arch"
    name = "openssl"
    default_user = "includeos"
    version = "1.1.1"

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
        self.requires("libcxx/[>=5.0]".format(self.user,self.channel))

    # TODO: Replaced by deployers in conan2
    # https://docs.conan.io/2/reference/extensions/deployers.html
    def imports(self):
        copy(self,"*",dst="include",src="include")

    def source(self):
        tag="OpenSSL_"+self.version.replace('.','_')
        repo = Git(self)
        a = ["--branch",tag]
        repo.clone("https://github.com/openssl/openssl.git",target="openssl", args=a)

    def build(self):
        options=["no-ssl3"]
        if self.options["ubsan"]:
            options+=['enable-ubsan'] # Looks like you need to link against ubsan regardless.
        if not self.options["threads"]:
            options+=['no-threads']
        if not self.options["shared"]:
            options+=['no-shared']
        if not self.options["async"]:
            options+=['no-async']
        if str(self.settings.arch) == "x86":
            options+=['386']

        options+=["-Iinclude/c++/v1","-Iinclude"]
        self.run(("./config --prefix="+self.package_folder+" --openssldir="+self.package_folder+" ".join(options)),cwd="openssl" )
        self.run("make -j depend",cwd="openssl")
        self.run("make -j",cwd="openssl")

    def package(self):
        pkg_inc=os.path.join(self.package_folder,"include/openssl")
        src_inc=os.path.join(self.source_folder,"openssl","include","openssl")
        pkg_lib=os.path.join(self.package_folder, "lib")
        src_lib=os.path.join(self.build_folder,"openssl")

        copy(self, pattern="*.h",dst=pkg_inc,src=src_inc)
        copy(self, pattern="*.a",dst=pkg_lib, src=src_lib)

    def package_info(self):
        self.cpp_info.libs=['crypto','ssl']

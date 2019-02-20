import shutil
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

from conans import ConanFile,CMake,tools

class ProtobufConan(ConanFile):
    settings="os","compiler","build_type","arch"
    name = "protobuf"
    version = "3.5.1.1"
    options = {"threads":[True, False]}
    default_options = {"threads": True}

    license = 'Apache 2.0'
    description = 'A language-neutral, platform-neutral extensible mechanism for serializing structured data.'
    url = "https://developers.google.com/protocol-buffers/"

    def requirements(self):
        self.requires("libcxx/[>=5.0]@{}/{}".format(self.user,self.channel))## do we need this or just headers

    def imports(self):
        self.copy("*",dst="include",src="include")

    def source(self):
        repo = tools.Git(folder="protobuf")
        repo.clone("https://github.com/google/protobuf.git",branch="v"+str(self.version))

    def _target_triple(self):
        if (str(self.settings.arch) == "x86"):
            return "i386-pc-linux-gnu"
        return str(self.settings.arch)+"-pc-linux-gnu"

    def build(self):
        cmake=CMake(self) #AutoToolsBuildEnvironment(self)
        cflags=[]

        if self.settings.build_type == "Debug":
            cflags+=["-g"]

        if self.settings.compiler == "clang":
            cflags+=[" -target={} ".format(self._target_triple())]

        if str(self.settings.arch) == "x86":
            cflags+=['-m32']
        #TODO fix cflags
        cflags+=['-msse3','-mfpmath=sse']
        ##how to pass this properly to cmake..
        cflags+=["-I"+self.build_folder+"/include/c++/v1","-I"+self.build_folder+"/include"]
        cmake.definitions['CMAKE_CROSSCOMPILING']=True
        #cmake.definitions['CMAKE_C_FLAGS']=" ".join(cflags)
        #cmake.definitions['CMAKE_CXX_FLAGS']=" ".join(cflags)
        cmake.definitions['CMAKE_USE_PTHREADS_INIT']=self.options.threads
        cmake.definitions['protobuf_VERBOSE']='ON'
        cmake.definitions['protobuf_BUILD_TESTS']='OFF'
        cmake.definitions['protobuf_BUILD_EXAMPLES']='OFF'
        cmake.definitions['BUILD_SHARED_LIBS']=''
        cmake.definitions['protobuf_WITH_ZLIB']=''
        cmake.configure(source_folder="protobuf/cmake")

        cmake.build(target="libprotobuf-lite")
        cmake.build(target="libprotobuf")
        #cmake.install(args=['libprotobuf','libprotobuf-lite','protobuf-headers'])
    def package(self):
        self.copy("*.a",dst="lib",src=".")
        self.copy("*.h",dst="include/protobuf",src=self.source_folder+"/protobuf/src/google/protobuf",excludes=['testdata','testing'])

    def deploy(self):
        self.copy("*.h",src="include",dst="include")
        self.copy("*.a",src="lib",dst="lib")

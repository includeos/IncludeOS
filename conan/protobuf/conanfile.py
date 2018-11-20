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
    #options = {"shared":False}
    #branch = "version"+version
    license = 'Apache 2.0'
    description = 'A language-neutral, platform-neutral extensible mechanism for serializing structured data.'
    url = "https://developers.google.com/protocol-buffers/"
    #keep_imports=True


    def build_requirements(self):
        self.build_requires("binutils/2.31@includeos/stable")
        self.build_requires("musl/v1.1.18@includeos/stable")
        self.build_requires("libcxx/[>=5.0]@{}/{}".format(self.user,self.channel))## do we need this or just headers

    def imports(self):
        self.copy("*",dst="target/include",src=self.deps_cpp_info["musl"].include_paths[0])
        self.copy("*",dst="target/libcxx/include",src=self.deps_cpp_info["libcxx"].include_paths[0]+"/c++/v1")
        #self.copy("*",dst="target/libunwind",src="libunwind")


    def source(self):
        repo = tools.Git(folder="protobuf")
        repo.clone("https://github.com/google/protobuf.git",branch="v"+str(self.version))

    def build(self):
        threads=''
        if self.options.threads:
            threads='ON'

        env_inc=" -I"+self.build_folder+"/target/libcxx/include -I"+self.build_folder+"/target/include "
        cmake=CMake(self) #AutoToolsBuildEnvironment(self)

        cflags="-msse3 -g -mfpmath=sse"
        cxxflags=cflags

        #if (self.settings.compiler == "clang" ):
        #    cflags+=" -nostdlibinc -nostdinc" # do this in better python by using a list
        #if (self.settings.compiler == "gcc" ):
        #    cflags+=" -nostdinc "

        cxxflags+=env_inc
        cflags+=env_inc

        #cmake.definitions["CMAKE_C_FLAGS"] = cflags
        #cmake.definitions["CMAKE_CXX_FLAGS"] = cxxflags
        cmake.definitions['CMAKE_USE_PTHREADS_INIT']=threads
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

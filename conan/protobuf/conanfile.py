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

    default_options = {"threads": False}
    #options = {"shared":False}
    #branch = "version"+version
    license = 'Apache 2.0'
    description = 'A language-neutral, platform-neutral extensible mechanism for serializing structured data.'
    url = "https://developers.google.com/protocol-buffers/"
    #exports_sources=['protobuf-options.cmake']
    keep_imports=True
    #keep_source=True #TODO REMOVE
    def build_requirements(self):
        self.build_requires("binutils/2.31@includeos/stable")
        self.build_requires("musl/v1.1.18@includeos/stable")
        self.build_requires("llvm/5.0@includeos/stable")## do we need this or just headers

    def imports(self):
        #if we copy everythint to include and lib everything "should" just work ?
        #self.copy("*",dst="lib",src="libcxx/lib")
        #self.copy("*",dst="lib",src="lib")
        #self.copy("*",dst="include",src="libcxx/include")
        #self.copy("*",dst="include",src="include")
        self.copy("*",dst="target",src=".")

    def source(self):
        repo = tools.Git(folder="protobuf")
        repo.clone("https://github.com/google/protobuf.git")
        self.run("git fetch --all --tags --prune",cwd="protobuf")
        self.run("git checkout tags/v"+str(self.version)+" -b "+str(self.version),cwd="protobuf")
        #shutil.copy("protobuf-options.cmake","protobuf/cmake/protobuf-options.cmake")
        #self.run("git submodule update --init --recursive",cwd="protobuf")
    def build(self):
        #self.cpp_info.cflags["-nostdinc"]
        #self.cpp_info.cppflags["-nostdinc"]
        threads=''
        if self.options.threads:
            threads='ON'

        env_inc="  -I"+self.build_folder+"/target/libcxx/include -I"+self.build_folder+"/target/include"+""
        cmake=CMake(self) #AutoToolsBuildEnvironment(self)

        cflags="-msse3 -g -mfpmath=sse -D_LIBCPP_HAS_MUSL_LIBC"
        cxxflags=cflags

        if (self.settings.compiler == "clang" ):
            cflags+=" -nostdlibinc -nostdinc" # do this in better python by using a list
        if (self.settings.compiler == "gcc" ):
            cflags+=" -nostdinc "

        #cflags+=" -I"+self.build_folder+"/include "
        cxxflags+=env_inc
        cflags+=env_inc
        #doesnt cmake have a better way to pass the -I params ?

        #    cmake.definitions["CMAKE_C_FLAGS"] =cflags+" -D_LIBCPP_HAS_MUSL_LIBC -I"+musl+" -I"+llvm_source+"/posix -I"+llvm_source+"/projects/libcxx/include -I"+llvm_source+"/projects/libcxxabi/include -Itarget/include"
        #    cmake.definitions["CMAKE_CXX_FLAGS"] =cxxflags+" -D_LIBCPP_HAS_MUSL_LIBC -I"+musl+" -I"+llvm_source+"/posix -I"+llvm_source+"/projects/libcxx/include -I"+llvm_source+"/projects/libcxxabi/include"

        cmake.definitions["CMAKE_C_FLAGS"] = cflags
        cmake.definitions["CMAKE_CXX_FLAGS"] = cxxflags
        cmake.definitions['CMAKE_USE_PTHREADS_INIT']=threads
        cmake.definitions['protobuf_VERBOSE']='ON'
        cmake.definitions['protobuf_BUILD_TESTS']='OFF'
        cmake.definitions['protobuf_BUILD_EXAMPLES']='OFF'
        cmake.definitions['BUILD_SHARED_LIBS']=''
        cmake.definitions['protobuf_WITH_ZLIB']=''
        cmake.configure(source_folder="protobuf/cmake")
        #git submodule update --init --recursive
        #cmake.definitions["CMAKE_CXX_FLAGS"]="nostdinc"
        # ?? protobuf_WITH_ZLIB
        #env_build.configure(configure_dir="protobuf")
        #libprotobuf or libprotobuf lite ??
        cmake.build(target="libprotobuf-lite")
        cmake.build(target="libprotobuf")
        #cmake.build(target="libprotoc")

    def package(self):
        print("TODO")
        #todo extract to includeos/include!!
        #self.copy("*",dst="include/rapidjson",src="rapidjson/include/rapidjson")

    def deploy(self):
        print("TODO")
        #self.copy("*",dst="include/rapidjson",src="include/rapidjson")

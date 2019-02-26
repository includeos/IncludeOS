from conans import ConanFile

class MuslConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "precompiled"
    #version = "0.13.0"
    license = 'Apache'
    description = 'IncludeOS C++ unikernel c/c++ libraries'
    url = "http://www.includeos.org"

    def build_requirements(self):
        self.build_requires("musl/v1.1.18@%s/%s"%(self.user,self.channel))
        self.build_requires("libgcc/1.0@%s/%s"%(self.user,self.channel))
        self.build_requires("llvm/5.0@%s/%s"%(self.user,self.channel))
        self.build_requires("libgcc/1.0@%s/%s"%(self.user,self.channel))

    def imports(self):
        path="PrecompiledLibraries/"+str(self.settings.arch)

        musl_lib=self.deps_cpp_info["musl"].lib_paths[0]
        musl_include=self.deps_cpp_info["musl"].include_paths[0]

        self.copy("*",dst=path+"/musl/lib",src=musl_lib)
        self.copy("*",dst=path+"/musl/include",src=musl_include)
        self.copy("*",dst=path+"/libgcc",src="libgcc")
        self.copy("*",dst=path+"/libcxx",src="libcxx")
        self.copy("*",dst=path+"/libunwind",src="libunwind")

    def build(self):
        self.run("tar -czf PrecompiledLibraries.tgz ./PrecompiledLibraries")


    def package(self):
        self.copy("*",dst=".",src=".")

    def deploy(self):
        self.copy("PrecompiledLibraries.tgz",dst=".",src=".")

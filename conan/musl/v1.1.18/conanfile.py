import os
import shutil

from conans import ConanFile,tools,AutoToolsBuildEnvironment

class MuslConan(ConanFile):
    #what makes the generated package unique..
    #compiler .. target .. and build type
    settings= "compiler","arch","build_type","arch_build"
    name = "musl"
    version = "v1.1.18"
    license = 'MIT'
    description = 'musl - an implementation of the standard library for Linux-based systems'
    url = "https://www.musl-libc.org/"

    def imports(self):
        self.copy("*",dst=".",src=".")

    def build_requirements(self):
        self.build_requires('binutils/2.31@{}/{}'.format(self.user,self.channel))

    def source(self):
        git = tools.Git(folder="musl")
        git.clone("https://github.com/includeos/musl.git",branch="master")

        # Replace syscall API's
        os.unlink("musl/arch/x86_64/syscall_arch.h")
        os.unlink("musl/arch/i386/syscall_arch.h")

    def _find_arch(self):
        if str(self.settings.arch) == "x86_64":
            return "x86_64"
        if str(self.settings.arch) == "x86" :
            return "i386"
        raise ConanInvalidConfiguration("Binutils no valid target for {}".format(self.settings.arch))

    def _find_host_arch(self):
        if str(self.settings.arch_build) == "x86":
            return "i386"
        return str(self.settings.arch_build)

    def build(self):
        host = self._find_host_arch()+"-pc-linux-gnu"
        triple = self._find_arch()+"-elf"
        args=[
            "--disable-shared",
            "--disable-gcc-wrapper"
        ]
        if (self.settings.build_type == "Debug"):
            args.append("--enable-debug")
        env_build = AutoToolsBuildEnvironment(self)
        if str(self.settings.compiler) == 'clang':
            env_build.flags=["-g","-target {}-pc-linux-gnu".format(self._find_arch())]
        #TODO fix this is only correct if the host is x86_64
        if str(self.settings.compiler) == 'gcc':
            if self._find_arch() == "x86_64":
                env_build.flags=["-g","-m64"]
            if self._find_arch() == "i386":
                env_build.flags=["-g","-m32"]

        if self.settings.build_type == "Debug":
            env_build.flags+=["-g"]
        env_build.configure(configure_dir=self.source_folder+"/musl",
            host=host,
            target=triple,
            args=args) #what goes in here preferably
        env_build.make()
        env_build.install()

    def package(self):
        self.copy("*.h",dst="include",src="musl/include")
        self.copy("includeos_syscalls.h",dst="include",src="musl/src/internal")
        self.copy("*.a",dst="lib",src="lib")
        self.copy("*.o",dst="lib",src="lib")

    def package_info(self):
        #default is ok self.cpp_info.includedirs
        self.cpp_info.libs=['c','crypt','m','rt','dl','pthread','resolv','util','xnet',]
        #what about crt1 crti crtn rcrt1 scrt1 '.o's
        self.cpp_info.libdirs=['lib']
        #where it was buildt doesnt matter
        self.info.settings.os="ANY"
        #what libcxx the compiler uses isnt of any known importance
        self.info.settings.compiler.libcxx="ANY"
        #we dont care what arch you buildt it on only which arch you want to run on
        self.info.settings.arch_build="ANY"
        self.info.settings.cppstd="ANY"

    def deploy(self):
        self.copy("*.h",dst="include",src="include")
        self.copy("*.a",dst="lib",src="lib")
        self.copy("*.o",dst="lib",src="lib")

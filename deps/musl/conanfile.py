import os
import shutil

from conan import ConanFile, tools
from conan.tools.files import patch
from conan.tools.files import copy
from conan.tools.scm import Git
from conan.tools.gnu import Autotools, AutotoolsToolchain

class MuslConan(ConanFile):
    settings= "compiler","arch","build_type","os"
    name = "musl"
    default_user = "includeos"
    version = "v1.1.18"
    license = 'MIT'
    description = 'musl - an implementation of the standard library for Linux-based systems'
    url = "https://www.musl-libc.org/"

    exports_sources = "patches*"

    # def build_requirements(self):
        # self.build_requires("binutils/2.31")

    def imports(self):
        print("imports")
        triple = str(self.settings.arch)+"-elf"
        tgt=triple+"-elf"
        self.copy("*",dst="bin",src="bin") #copy binaries..
        self.copy("*.a",dst="lib",src="lib")
        self.copy("*",dst=tgt,src=tgt)


    def source(self):
        print("source")
        git = Git(self)
        folder="musl"
        clone_args=["--branch", self.version]
        git.clone("git://git.musl-libc.org/musl/", args=clone_args)

        # Replace syscall API
        patch(self, base_path="musl", patch_file="patches/musl.patch")
        patch(self, base_path="musl", patch_file="patches/endian.patch")

        dest_syscalls = os.path.join('musl', 'src', 'internal')
        copy(self, pattern="includeos_syscalls.h", src="patches", dst=dest_syscalls)
        copy(self, pattern="syscall.h", src="patches", dst=dest_syscalls)

        os.unlink("musl/arch/x86_64/syscall_arch.h")
        os.unlink("musl/arch/i386/syscall_arch.h")

    def generate(self):
        tc = AutotoolsToolchain(self)
        tc.generate()

    def build(self):
        print("build")
        print("Build to folder: {}".format(self.build_folder))
        triple = str(self.settings.arch)+"-elf"

        #TODO swap this to use self.settings.arch
        autotools = Autotools(self)

        # TODO: Needed with clang18 this warning is turned into an error.
        # The origin of the issue are some of the syscalls converting address to long
        # for example, musl/src/internal/pthread_impl.h:137:23
        #  __syscall(SYS_futex, addr, FUTEX_WAKE, cnt);
        #                       ^~~~
        # which becomes
        # extern long syscall_SYS_futex(long, ...);
        #
        cflags='CFLAGS=-Wno-error=int-conversion'
        autotools.configure(build_script_folder="musl",
                            args=["--enable-debug", "--disable-shared",cflags])
        autotools.make(args=["-j"])
        autotools.install()

    def package(self):
        print("Packaging to folder: {}".format(self.package_folder))
        include_src = os.path.join(self.source_folder, "musl", "include")
        lib_pkg = os.path.join(self.package_folder, "lib")
        lib_bld = os.path.join(self.build_folder, "lib")
        copy(self, pattern="*.h",dst="include",src="musl/include")
        copy(self, pattern="*.a",dst=lib_pkg, src=lib_bld)
        copy(self, pattern="*.o",dst=lib_pkg, src=lib_bld)


    def package_info(self):
        self.cpp_info.includedirs = ["include"]

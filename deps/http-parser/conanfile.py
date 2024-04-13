import os
from conan import ConanFile,tools
from conan.tools.scm import Git
from conan.tools.files import copy

class HttpParserConan(ConanFile):
    settings="os","compiler","build_type","arch"
    name = "http-parser"
    version = "2.8.1"
    license = 'MIT'
    description = 'This is a parser for HTTP messages written in C'
    url = "https://github.com/nodejs/http-parser"

    def source(self):
        repo = Git(self)
        clone_args = ['--branch', "v{}".format(self.version)]        
        repo.clone("https://github.com/nodejs/http-parser.git",
                   target="http_parser")

    #TODO handle target flags
    def configure(self):
        #doesnt matter what stdc++ lib you have
        del self.settings.compiler.libcxx
        
    def build(self):
        #TODO improve this to support multi platform and multi tooling
        #probably easiest just to supply a cmake with conan
        self.run("make http_parser.o",cwd="http_parser")
        self.run("ar rcs libhttp-parser.a http_parser.o",cwd="http_parser")

    def package(self):
        source = os.path.join(self.source_folder, "http_parser")
        inc    = os.path.join(self.package_folder, "include", "http-parser")
        lib    = os.path.join(self.package_folder, "lib")
        copy(self, pattern="*.h", dst=inc,src=source)
        copy(self, pattern="http_parser.o", dst=lib, src=source)
        copy(self, pattern="*.a", dst=lib, src=source)

    def package_info(self):
        self.cpp_info.libs=['http-parser']

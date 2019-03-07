#version 2.8.1 is the latest tag on time of writing
from conans import ConanFile,tools

class HttpParserConan(ConanFile):
    settings="os","compiler","build_type","arch"
    name = "http-parser"
    #version = "2.9"
    license = 'MIT'
    description = 'This is a parser for HTTP messages written in C'

    url = "https://github.com/nodejs/http-parser"

    def source(self):
        repo = tools.Git(folder="http_parser")
        repo.clone("https://github.com/nodejs/http-parser.git",branch="v{}".format(self.version))
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
        self.copy("*.h",dst="include/http-parser",src="http_parser")
        self.copy("http_parser.o",dst="lib",src="http_parser")
        self.copy("*.a",dst="lib",src="http_parser")

    def package_info(self):
        self.cpp_info.libs=['http-parser']
    def deploy(self):
        self.copy("*.o",dst="lib",src="lib")
        self.copy("*.h",dst="include",src="include")

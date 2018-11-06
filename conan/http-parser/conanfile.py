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
        repo.clone("https://github.com/nodejs/http-parser.git")
        self.run("git fetch --all --tags --prune",cwd="http_parser")
        self.run("git checkout tags/v"+str(self.version)+" -b "+str(self.version),cwd="http_parser")
    def build(self):
        self.run("make http_parser.o",cwd="http_parser")

    def package(self):
        self.copy("*.h",dst="include",src="http_parser")
        self.copy("http_parser.o",dst="lib",src="http_parser")

    def deploy(self):
        self.copy("*.o",dst="lib",src="lib")
        self.copy("*.h",dst="include",src="include")

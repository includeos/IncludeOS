import os
from conan import ConanFile,tools
from conan.tools.scm import Git
from conan.tools.files import copy

class RapidJsonConan(ConanFile):
    name = "rapidjson"
    version = "1.1.0"
    license = 'MIT'
    description = 'A fast JSON parser/generator for C++ with both SAX/DOM style API'
    url = "https://github.com/Tencent/rapidjson/"

    def source(self):
        repo = Git(self)
        clone_args = ['--branch', "v{}".format(self.version)]        
        repo.clone("https://github.com/Tencent/rapidjson.git",args=clone_args)

        
    def package(self):
        source = os.path.join(self.source_folder, "rapidjson", "include")
        dest   = os.path.join(self.package_folder, "include")
        print("DEBUG: src: {}\ndst: {}".format(source, dest))
        copy(self, pattern="*", src=source, dst=dest)


import shutil

from conans import ConanFile,tools

class RapidJsonConan(ConanFile):
    name = "rapidjson"
    version = "1.1.0"
    branch = "version"+version
    license = 'MIT'
    description = 'A fast JSON parser/generator for C++ with both SAX/DOM style API'
    url = "https://github.com/Tencent/rapidjson/"

    def source(self):
        repo = tools.Git(folder="rapidjson")
        repo.clone("https://github.com/Tencent/rapidjson.git",branch=str(self.branch))

    #def build(self):
#        shutil.copy("rapidjson/include")

    def package(self):
        #todo extract to includeos/include!!
        self.copy("*",dst="include",src="rapidjson/include/rapidjson")

    def deploy(self):
        self.copy("*",dst="include",src="include")

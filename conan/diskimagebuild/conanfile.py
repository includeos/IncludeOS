from conans import ConanFile,tools,CMake

class DiscImagebuildConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "diskimagebuild"
    license = 'Apache-2.0'
    description = 'Run your application with zero overhead'
    generators = 'cmake'
    url = "http://www.includeos.org/"
    def source(self):
        repo = tools.Git(folder="includeos")
        repo.clone("https://github.com/hioa-cs/IncludeOS.git",branch="master")

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder=self.source_folder+"/IncludeOS/diskimagebuild")
        cmake.build()
        cmake.install()


    def deploy(self):
        self.copy("diskbuilder",dst="bin",src=".")

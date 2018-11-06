from conans import ConanFile,tools

class UzlibConan(ConanFile):
    settings="os","compiler","build_type","arch"
    name = "uzlib"
    version = "2.9"
    license = 'zlib'
    description = 'uzlib - Deflate/Zlib-compatible LZ77 compression/decompression library'
    url = "http://www.ibsensoftware.com/"

    def source(self):
        repo = tools.Git(folder="uzlib")
        repo.clone("https://github.com/pfalcon/uzlib")
        self.run("git fetch --all --tags --prune",cwd="uzlib")
        self.run("git checkout tags/v"+str(self.version)+" -b "+str(self.version),cwd="uzlib")
    def build(self):
        self.run("make -j20",cwd="uzlib")

    def package(self):
        self.copy("*.h",dst="include",src="uzlib")
        self.copy("*.a",dst="lib",src="uzlib")

    def deploy(self):
        self.copy("*.a",dst="lib",src="lib")
        self.copy("*.h",dst="include",src="include")

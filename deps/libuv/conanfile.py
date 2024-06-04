from conans import ConanFile,tools, CMake

class LibuvConan(ConanFile):
    settings= "os","arch","build_type","compiler"
    name = "libuv"
    version="1.27.0"
    license = 'MIT'
    description = 'Cross-platform async IO'
    url = "http://www.includeos.org/"

    default_user="includeos"
    default_channel="test"

    def source(self):
        repo = tools.Git(folder="libuv")
        repo.clone("https://github.com/libuv/libuv.git",branch="v1.27.0")

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure(source_folder="libuv")
        return cmake

    def build(self):
        cmake=self._configure_cmake()
        cmake.build()

    def deploy(self):
        self.copy("*.a",dst="lib",src=".libs")
        self.copy("*.h",dst="include",src="include")

    def package(self):
        cmake=self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs=['uv_a']

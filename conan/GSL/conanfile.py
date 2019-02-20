from conans import ConanFile,tools
#mark up GSL/Version@user/channel when building this one
#instead of only user/channel
#at the time of writing 1.0.0 and 2.0.0 are valid versions

class GslConan(ConanFile):
    name = "GSL"
    license = 'MIT'
    description = 'GSL: Guideline Support Library'
    url = "https://github.com/Microsoft/GSL"
    no_copy_source=True

    def source(self):
        repo = tools.Git()
        repo.clone(self.url +".git",branch="v{}".format(self.version))
        
    def buld(self):
        skip

    def package(self):
        self.copy("*",dst="include",src="include")

    def deploy(self):
        self.copy("*",dst="include",src="include")

    def package_id(self):
        self.info.header_only()

from conans import ConanFile,tools

#mark up GSL/Version@user/channel when building this one
#instead of only user/channel
#at the time of writing 1.0.0 and 2.0.0 are valid versions

class GslConan(ConanFile):
    name = "GSL"
    license = 'MIT'
    description = 'GSL: Guideline Support Library'
    url = "https://github.com/Microsoft/GSL"

    def source(self):
        repo = tools.Git()
        repo.clone("https://github.com/Microsoft/GSL.git")
        self.run("git fetch --all --tags --prune")
        self.run("git checkout tags/v"+str(self.version)+" -b "+str(self.version))


    def package(self):
        #todo extract to includeos/include!!
        self.copy("*",dst="include/gsl",src="include/gsl")

    def deploy(self):
        self.copy("*",dst="include/gsl",src="include/gsl")

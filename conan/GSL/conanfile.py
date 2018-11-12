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
        #TODO FIXME THIS IS BAD
        self.run("git checkout 9d13cb14c3cf6b59bd16071929f25ac5516a4d24 -b "+str(self.version))
        #self.run("git checkout tags/v"+str(self.version)+" -b "+str(self.version))


    def package(self):
        #todo extract to includeos/include!!
        self.copy("*",dst="include/gsl",src="gsl")

    def deploy(self):
        self.copy("*",dst="include/gsl",src="include/gsl")

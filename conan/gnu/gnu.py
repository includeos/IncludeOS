from conans import ConanFile,tools
class GnuConan(ConanFile):
    license = "GNU GPL"
    def source(self):
        zip_name="%s-%s.tar.gz"%(self.name,self.version)
        tools.download('https://ftp.gnu.org/gnu/%s/%s'%(self.name,zip_name),zip_name)
        tools.unzip(zip_name)

    def package(self):
        self.copy("*",dst=self.name,src=self.source_folder+'/'+self.name+"-"+self.version)

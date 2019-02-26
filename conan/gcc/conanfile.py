from conans import ConanFile,tools

class GccBaseConan(ConanFile):
    license = "GNU GPL"
    compression='gz'
    def source(self):
        zip_name="%s-%s.tar.%s"%(self.name,self.version,self.compression)
        tools.download('ftp://gcc.gnu.org/pub/gcc/infrastructure/%s'%zip_name,zip_name)
        tools.unzip(zip_name)

    def package(self):
        self.copy("*",dst=self.name,src=self.source_folder+'/'+self.name+"-"+self.version)

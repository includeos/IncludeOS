
from conans import ConanFile,tools

class NaClConan(ConanFile):
    name = 'nacl'
    version="v0.2.0"
    license = 'Apache-2.0'
    description='NaCl is a configuration language for IncludeOS that you can use to add for example interfaces and firewall rules to your service.'
    url='https://github.com/includeos/NaCl'

    def source(self):
        repo = tools.Git()
        repo.clone("https://github.com/includeos/NaCl.git",branch=self.version)
    def build(self):
        #you need antlr4 installed to do this
        self.run("antlr4 -Dlanguage=Python2 NaCl.g4 -visitor")

    def package(self):
        name='NaCl'
        self.copy('*',dst=name+"/subtranspilers",src="subtranspilers")
        self.copy('*',dst=name+"/type_processors",src="type_processors")
        self.copy('*.py',dst=name,src=".")
        self.copy('cpp_template.mustache',dst=name,src='.')
        self.copy('NaCl.tokens',dst=name,src=".")
        self.copy('NaClLexer.tokens',dst=name,src=".")

    def package_id(self):
        self.info.header_only()
        
    def deploy(self):
        self.copy("*",dst="NaCl",src="NaCl")

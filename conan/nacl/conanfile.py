
from conans import ConanFile,tools

class NaClConan(ConanFile):
    #settings = ''
    name = 'nacl'
    version="v0.1.0"
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
        self.copy('NaCl.tokens',dst="NaCl",src=".")
        self.copy('NaClLexer.py',dst="NaCl",src=".")
        self.copy('NaClLexer.tokens',dst="NaCl",src=".")
        self.copy('NaClListener.py',dst="NaCl",src=".")
        self.copy('NaClParser.py',dst="NaCl",src=".")
        self.copy('NaClVisitory.py',dst="NaCl",src=".")
    def deploy(self):
        self.copy("*",dst="NaCl",src="NaCl")

#!/bin/python
import os, sys
import platform

llvm_versions=['5.0','6.0','7.0']
#passing -a to arch will replace the last value!!
profiles= [
#  'clang-6.0-x86_64-linux-i386',
  'clang-6.0-x86_64-linux-x86_64',
#  'clang-6.0-x86_64-linux-x86_64',
  'gcc-8.2.0-x86_64-linux-x86_64',
#  'gcc-7.3.0-x86_64-linux-x86_64'
]
channel='includeos/test'

def system(command):
    retcode = os.system(command)
    if (retcode != 0):
        raise Exception("error while executing: {}\n\t".format(command))

#hmm replace profile with args .. -s compiler.version= -s compiler= -s arch= and generate the actual profiles ??
def create(name='',path='',version='',userchan='',args='',profile=''):
    if version != '':
        userchan=name+"/"+version+"@"+userchan
    system('conan create '+path+' '+userchan+' '+args+' -pr '+profile)

#todo create json list ?

#create toolchains
for p in profiles:
    create('binutils',"gnu/binutils/2.31",'',channel,'',p+"-toolchain")

tools_version="0.13.0"
#non profile based tools that run on the host!!
create('vmbuild',"vmbuild",tools_version,channel,'','default')
create('diskimagebuild',"diskimagebuild",tools_version,channel,'','default')

#non profile based packages
create('rapidjson','rapidjson','',channel,'','default')
create('GSL','GSL','2.0.0',channel,'','default')
for profile in profiles:
    create('musl',"musl/v1.1.18",'',channel,'',p)
    for version in llvm_versions:
        create('libcxxabi',"llvm/libcxxabi",version,channel,'',p)
        create('libunwind',"llvm/libunwind",version,channel,'',p)
        create('libcxx',"llvm/libcxx",version,channel,'',p)
    create('botan','botan','2.8.0',channel,'',p)
    create('openssl','openssl/1.1.1','',channel,'',p)
    create('s2n','s2n','1.1.1',channel,'',p)
    create('protobuf','protobuf','3.5.1.1',channel,'',p)

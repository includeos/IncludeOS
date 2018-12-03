#!/bin/python

import os, sys
import platform

userchan="includeos/test"

llvm_versions=['5.0','6.0','7.0']
llvm_components=['libcxx','libcxxabi','libunwind']


def system(command):
    retcode = os.system(command)
    if (retcode != 0):
        raise Exception("error while executing: {}\n\t".format(command))

osv='0.13.0'
#export includeos
system('conan export includeos includeos/{}@{} --keep-source'.format(osv,userchan))

for v in llvm_versions:
    for c in llvm_components:
        system('conan export llvm/{} {}/{}@{} --keep-source'.format(c,c,v,userchan))

print ('export complete"')

